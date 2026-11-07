#include "hook.h"

#include <cstdlib>
#include <cstring>
#include <new>

#include "../Utils/util.h"
#include "schedule.h"

/*
 * Declaration private functions
 */
bool Hooks_deserialize_hook_rest(Hook &dest, uint8_t **encoded_hook,
                                 uint8_t &error_code);

bool Hooks_deserialize_state_to_inspect(Hook &dest, uint8_t **encoded_hook,
                                        uint8_t &error_code);

/*
 * Hook pool
 *
 * Hooks are allocated in chunks to avoid the malloc overhead (header and
 * alignment) of allocating each hook individually. Free hooks are kept in a
 * free list linked via `nextHook`. Chunks grow geometrically so that a few
 * hooks only take a little memory (relevant for MCUs) while many hooks
 * amortise the chunk allocations. Once no hook is in use anymore, all chunks
 * are released.
 */
namespace {
struct HookChunk {
    HookChunk *next;
};

constexpr uint32_t HOOK_POOL_MIN_CHUNK_SIZE = 16;
constexpr uint32_t HOOK_POOL_MAX_CHUNK_SIZE = 4096;
constexpr size_t HOOK_CHUNK_HEADER_SIZE =
    (sizeof(HookChunk) + alignof(Hook) - 1) / alignof(Hook) * alignof(Hook);

HookChunk *hookChunks = nullptr;
Hook *freeHooks = nullptr;
uint32_t nextChunkSize = HOOK_POOL_MIN_CHUNK_SIZE;
size_t hooksInUse = 0;

bool hookPoolGrow() {
    uint32_t nrHooks = nextChunkSize;
    void *mem = malloc(HOOK_CHUNK_HEADER_SIZE + nrHooks * sizeof(Hook));
    if (mem == nullptr) {
        return false;
    }
    HookChunk *chunk = static_cast<HookChunk *>(mem);
    chunk->next = hookChunks;
    hookChunks = chunk;
    Hook *hooks = reinterpret_cast<Hook *>(static_cast<uint8_t *>(mem) +
                                           HOOK_CHUNK_HEADER_SIZE);
    for (uint32_t i = nrHooks; i > 0; i--) {
        Hook *h = new (&hooks[i - 1]) Hook();
        h->nextHook = freeHooks;
        freeHooks = h;
    }
    if (nextChunkSize < HOOK_POOL_MAX_CHUNK_SIZE) {
        nextChunkSize *= 2;
    }
    return true;
}

void hookPoolRelease() {
    while (hookChunks != nullptr) {
        HookChunk *next = hookChunks->next;
        free(hookChunks);
        hookChunks = next;
    }
    freeHooks = nullptr;
    nextChunkSize = HOOK_POOL_MIN_CHUNK_SIZE;
}

void hookPoolReturn(Hook *hook) {
    hook->nextHook = freeHooks;
    freeHooks = hook;
    hooksInUse -= 1;
    if (hooksInUse == 0) {
        hookPoolRelease();
    }
}

/*
 * Shared states to inspect. The client requests the states sorted, so a
 * request for the pc and the stack always arrives as [pc, stack].
 */
ExecutionState sharedPCOnly[] = {pcState};
ExecutionState sharedPCAndStack[] = {pcState, stackState};
StateToInspect makeSharedState(uint16_t nrOfInspects, ExecutionState *states) {
    StateToInspect s{};
    s.numberOfInspects = nrOfInspects;
    s.requestedState = states;
    return s;
}

StateToInspect sharedStatePC = makeSharedState(1, sharedPCOnly);
StateToInspect sharedStatePCAndStack = makeSharedState(2, sharedPCAndStack);

bool sameStates(const StateToInspect &s1, const StateToInspect &s2) {
    return s1.numberOfInspects == s2.numberOfInspects &&
           memcmp(s1.requestedState, s2.requestedState,
                  s1.numberOfInspects * sizeof(ExecutionState)) == 0;
}

StateToInspect *findSharedState(const StateToInspect &state) {
    for (StateToInspect *shared : {&sharedStatePC, &sharedStatePCAndStack}) {
        if (sameStates(*shared, state)) {
            return shared;
        }
    }
    return nullptr;
}

Hook makeSharedPauseHook() {
    Hook h{};
    h.id = 0;  // never observed: changing the running state sends no message
    h.kind = ChangeRunningState;
    h.once = false;
    h.value.runState = WARDUINOpause;
    h.nextHook = nullptr;
    return h;
}

const Hook sharedPauseHook = makeSharedPauseHook();
}  // namespace

/*
 * Public functions
 */
Hook *Hooks_new_hook() {
    if (freeHooks == nullptr && !hookPoolGrow()) {
        return nullptr;
    }
    Hook *hook = freeHooks;
    freeHooks = hook->nextHook;
    hooksInUse += 1;
    return new (hook) Hook();
}

bool Hooks_is_shared_state(const StateToInspect *state) {
    return state == &sharedStatePC || state == &sharedStatePCAndStack;
}

StateToInspect *Hooks_shared_state(bool includeStack) {
    return includeStack ? &sharedStatePCAndStack : &sharedStatePC;
}

bool Hooks_is_always_shared_inspect_hook(const Hook &hook) {
    return hook.kind == StateInspect && !hook.once &&
           Hooks_is_shared_state(hook.value.state);
}

void Hooks_init_shared_inspect_hook(Hook &dest, uint32_t id,
                                    bool includeStack) {
    dest.id = id;
    dest.kind = StateInspect;
    dest.once = false;
    dest.value.state = Hooks_shared_state(includeStack);
    dest.nextHook = nullptr;
}

const Hook &Hooks_shared_pause_hook() { return sharedPauseHook; }

bool Hooks_is_always_pause_hook(const Hook &hook) {
    return hook.kind == ChangeRunningState && !hook.once &&
           hook.value.runState == WARDUINOpause;
}

void Hooks_free_hook_value(Hook &hook) {
    switch (hook.kind) {
        case ValueSubstitution:
            delete hook.value.result;
            hook.value.result = nullptr;
            break;
        case StateInspect:
            if (!Hooks_is_shared_state(hook.value.state)) {
                Interrupt_Inspect_free_state_to_inspect(hook.value.state);
            }
            hook.value.state = nullptr;
            break;
        default:
            break;
    }
}

Hook *Hooks_add_and_sort(Hook *hooks, Hook *hook_to_add) {
    if (hook_to_add == nullptr) {
        return hooks;
    } else if (hooks == nullptr) {
        return hook_to_add;
    }

    // TODO sort hooks from more specific to more general
    // sorting goes from logical-clock, ev dep, cond, once, always
    // logical-clock < logical-clock < .. < logical-clock
    // -> sub sort ts before(s) .... ts on ts .... ts after
    // event dependency < event dep. < ... < event dep. condition
    // Once < once < ... < once
    // always (only one allowed)
    // BUT FOR NOW: just add hook at the end
    Hook *insert = hooks;
    while (insert->nextHook != nullptr) {
        insert = insert->nextHook;
    }
    if (insert->nextHook != nullptr) {
        hook_to_add->nextHook = insert->nextHook;
    }
    insert->nextHook = hook_to_add;
    return hooks;
}

Hook *Hooks_remove_completed_hook(Hook *hooks) {
    Hook *first_hook = hooks;
    Hook *prev_hook = nullptr;
    Hook *current_hook = first_hook;
    while (current_hook != nullptr) {
        Hook *nc = nullptr;
        bool remove = current_hook->once;

        if (remove) {
            nc = current_hook->nextHook;
            Hooks_free_hook(current_hook);
            current_hook = nc;
            if (prev_hook == nullptr) {
                first_hook = nc;
            } else {
                prev_hook->nextHook = current_hook;
            }
        } else {
            prev_hook = current_hook;
            current_hook = current_hook->nextHook;
        }
    }
    return first_hook;
}

Hook *Hooks_copyHook(Hook &hook) {
    switch (hook.kind) {
        case ProxyCall:
        case RemoteCall:
        case ValueSubstitution:
        case StateInspect:
        case ChangeRunningState:
        case EventAdd:
            break;
        default:
            return nullptr;
    }
    if (hook.kind == StateInspect && hook.value.state == nullptr) {
        return nullptr;
    }

    Hook *cpy = Hooks_new_hook();
    if (cpy == nullptr) {
        return nullptr;
    }
    // move the hook: resources owned by `hook` (e.g., the state to inspect or
    // the substitute value) are transferred to the copy
    *cpy = hook;
    cpy->nextHook = nullptr;
    switch (hook.kind) {
        case ValueSubstitution:
            hook.value.result = nullptr;
            break;
        case StateInspect:
            hook.value.state = nullptr;
            break;
        default:
            break;
    }
    return cpy;
}

void Hooks_free_hook(Hook *hook) {
    if (hook == nullptr || hook == &sharedPauseHook) {
        return;
    }
    Hooks_free_hook_value(*hook);
    hookPoolReturn(hook);
}

void Hooks_free_hooks(Hook *hook) {
    while (hook != nullptr) {
        Hook *hookToFree = hook;
        hook = hook->nextHook;
        Hooks_free_hook(hookToFree);
    }
}

/*
 * Private functions
 */
bool Hooks_deserialize_hook(Hook &dest, uint32_t id, uint8_t **encoded_hook,
                            uint8_t &error_code) {
    // format expected: Schedule | Hook
    dest.id = id;
    if (!Hooks_deserialize_schedule(dest.once, encoded_hook, error_code)) {
        return false;
    }
    return Hooks_deserialize_hook_rest(dest, encoded_hook, error_code);
}

bool Hooks_deserialize_schedule(bool &once, uint8_t **encoded_schedule,
                                uint8_t &error_code) {
    // format expected: SCHEDULE_KIND (1 BYTE)
    ScheduleKind schedule = (ScheduleKind) * *encoded_schedule;
    *encoded_schedule += 1;
    switch (schedule) {
        case ScheduleOnce:
            once = true;
            return true;
        case ScheduleAlways:
            once = false;
            return true;
        default:
            error_code = HOOK_ERROR_CODE_UNEXISTING_SCHEDULE_KIND;
            return false;
    }
}

bool Hooks_deserialize_hook_rest(Hook &dest, uint8_t **encoded_hook,
                                 uint8_t &error_code) {
    HookKind kind = (HookKind) * *encoded_hook;
    *encoded_hook += 1;
    switch (kind) {
        case ProxyCall:
            break;
        case RemoteCall:
            dest.value.target_fidx = read_LEB_32(encoded_hook);
            break;
        case ValueSubstitution: {
            bool hasValue = **encoded_hook;
            *encoded_hook += 1;
            if (hasValue) {
                ValueSerializationConfig config;
                config.includeType = true;
                config.includeIndex = false;
                size_t bytes_read = deserializeStackValue(
                    dest.value.result, config, *encoded_hook);
                if (bytes_read <= 0) {
                    error_code = HOOK_ERROR_CODE_SUBSTITUTE_VALUE_IS_MALFORMED;
                    return false;
                }
            } else {
                dest.value.result = nullptr;
            }
            break;
        }
        case StateInspect:
            if (!Hooks_deserialize_state_to_inspect(dest, encoded_hook,
                                                    error_code)) {
                return false;
            }
            break;
        case ChangeRunningState: {
            uint8_t newRunState = **encoded_hook;
            if (newRunState != WARDUINOrun && newRunState != WARDUINOpause) {
                error_code = HOOK_ERROR_CODE_UNSUPPORTED_RUNNING_STATE;
                return false;
            }
            dest.value.runState = (RunningState)newRunState;
            break;
        }
        case EventInspect:
        case EventRemove:
            break;
        case EventAdd: {
            Event e{"", ""};
            if (!binary_decode_event(e, *encoded_hook)) {
                error_code =
                    HOOK_ERROR_CODE_SUBSTITUTE_STATE_INSPECT_IS_MALFORMED;

                WARDuino::instance()->debugger->channel->write(
                    "failed deserialize eventadd\n");
                return false;
            }
            EventHook *ev =
                new EventHook();  // TODO perhaps move above to avoid malloc
            ev->payload = e.payload.c_str();
            ev->topic = e.topic.c_str();
            ev->payload_length = strlen(ev->payload);
            dest.value.ev = ev;
            break;
        }
        default:
            printf("HookKind %02X is not supported\n", kind);
            error_code = HOOK_ERROR_CODE_UNEXISTING_HOOK_KIND;
            return false;
    }
    dest.kind = kind;
    return true;
}
bool Hooks_deserialize_state_to_inspect(Hook &dest, uint8_t **encoded_hook,
                                        uint8_t &error_code) {
    dest.value.state = nullptr;
    StateToInspect parsed{};
    if (!Interrupt_Inspect_deserialize_request(parsed, *encoded_hook,
                                               error_code)) {
        free(parsed.requestedState);
        error_code = HOOK_ERROR_CODE_SUBSTITUTE_STATE_INSPECT_IS_MALFORMED;
        return false;
    }

    // Reuse a shared state when possible such that no memory is needed
    StateToInspect *shared = findSharedState(parsed);
    if (shared != nullptr) {
        free(parsed.requestedState);
        dest.value.state = shared;
        return true;
    }

    StateToInspect *state = Interrupt_Inspect_new_state_to_inspect();
    if (state == nullptr) {
        free(parsed.requestedState);
        error_code = HOOK_ERROR_CODE_INSUFFICIENT_MEMORY;
        return false;
    }
    *state = parsed;  // takes ownership of the requested states
    dest.value.state = state;
    return true;
}
