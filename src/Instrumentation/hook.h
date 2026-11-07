#pragma once
#include "../Debug/runningstate.h"
#include "../Interrupts/interrupt_inspect.h"
#include "../WARDuino/structs.h"
#include "./schedule.h"

#define HOOK_ERROR_CODE_SUBSTITUTE_VALUE_IS_MALFORMED 31;
#define HOOK_ERROR_CODE_UNEXISTING_HOOK_KIND 32;
#define HOOK_ERROR_CODE_UNEXISTING_SCHEDULE_KIND 33;
#define HOOK_ERROR_CODE_SUBSTITUTE_STATE_INSPECT_IS_MALFORMED 34;
#define HOOK_ERROR_CODE_INSUFFICIENT_MEMORY 35;
#define HOOK_ERROR_CODE_UNSUPPORTED_RUNNING_STATE 36;

// todo refactor parse event
// move error code
#define HOOK_ERROR_CODE_EVENT_ADD_FAIL_PARSE 37;
#define HOOK_ERROR_CODE_EVENT_ADD_EMPTY_TOPIC 38;

typedef struct EventHook {
    const char *topic{};
    const char *payload{};
    unsigned int payload_length{};
} EventHook;

// uint8_t backed so that the Hook struct stays compact
enum HookKind : uint8_t {
    RemoteCall = 0x01,
    ValueSubstitution = 0x02,
    StateInspect = 0x03,
    ChangeRunningState = 0x04,
    ProxyCall = 0x05,

    // event hooks from 0x10 to 0x1f
    EventInspect = 0x10,
    EventRemove = 0x11,  // disallow when hookmoment === HookAfterEventHandled,
                         // maybe also for OnEventHandling?
    EventAdd = 0x12,
};

/*
 * A hook is kept as small as possible (24 bytes on 64-bit targets) because
 * analyses such as instruction coverage register one or more hooks for every
 * instruction of the module. To that end:
 *  - the kind is stored in a single byte and the schedule is reduced to
 *    whether the hook runs once or always;
 *  - hooks are allocated from a pool (`Hooks_new_hook`/`Hooks_free_hook`)
 *    which avoids the per allocation overhead of malloc;
 *  - the commonly requested states to inspect (pc and pc + stack) are shared
 *    instances that are never freed (see `Hooks_is_shared_state`).
 *
 * The `id` is kept per hook as it is sent back to the client whenever the
 * hook runs (e.g., in the subscription message of a StateInspect).
 */
struct Hook {
    uint32_t id;
    HookKind kind;
    bool once{};  // true: removed after it ran, false: runs every time
    union {
        uint32_t target_fidx;
        StackValue *result{};
        StateToInspect *state;
        RunningState runState;
        EventHook *ev;
    } value;
    Hook *nextHook{};
};

/*
 * Allocates a (zero-initialised) hook from the hook pool. Hooks allocated via
 * this function must be released with `Hooks_free_hook`.
 */
Hook *Hooks_new_hook();

/*
 * Returns true if `state` is one of the shared states to inspect. Shared states
 * are referenced by many hooks and must therefore never be freed.
 */
bool Hooks_is_shared_state(const StateToInspect *state);

/*
 * Returns the shared state that inspects the pc and, if `includeStack`, the
 * stack.
 */
StateToInspect *Hooks_shared_state(bool includeStack);

/*
 * Returns true if the hook runs every time and inspects a shared state. Such
 * hook is fully described by its id and its shared state.
 */
bool Hooks_is_always_shared_inspect_hook(const Hook &hook);

/*
 * Initialises `dest` as a StateInspect hook, scheduled always, with id `id`
 * that inspects the shared state (see `Hooks_shared_state`).
 */
void Hooks_init_shared_inspect_hook(Hook &dest, uint32_t id,
                                    bool includeStack);

/*
 * Returns a shared hook that pauses the VM every time it runs
 * (kind ChangeRunningState, value WARDUINOpause, schedule always). The id of
 * such hook is never observed by the client so a single instance suffices.
 */
const Hook &Hooks_shared_pause_hook();

/*
 * Returns true if `hook` behaves identically to `Hooks_shared_pause_hook()`
 */
bool Hooks_is_always_pause_hook(const Hook &hook);

/*
 * Releases the resources owned by the hook value (e.g., a non-shared state to
 * inspect) without freeing the hook itself.
 */
void Hooks_free_hook_value(Hook &hook);

/*
 * sorts all the hooks based on the order they will be executed.
 * function assumes that `hooks` are already sorted
 */
Hook *Hooks_add_and_sort(Hook *hooks, Hook *hook_to_add);

/*
 * Moves `hook` into a newly allocated hook from the hook pool. Resources owned
 * by `hook` (e.g., the state to inspect) are transferred to the returned hook,
 * `hook` should therefore not be used anymore afterwards.
 */
Hook *Hooks_copyHook(Hook &hook);

typedef struct HooksRemoveResult {
    Hook *newList{};
    Hook *nextHook{};
} HooksRemoveResult;

/*
 * Removes from a hooks linked list `first_hook` the completed hook
 * `hook_completed` and returns the updated linked list
 */
Hook *Hooks_remove_completed_hook(Hook *hooks);

/*
 * Frees the hook and the resources it owns. Shared resources are not freed.
 */
void Hooks_free_hook(Hook *hook);

void Hooks_free_hooks(Hook *hook);

bool Hooks_deserialize_hook(Hook &dest, uint32_t id, uint8_t **encoded_hook,
                            uint8_t &error_code);

/*
 * Deserializes a schedule and stores into `once` whether the hook runs once.
 */
bool Hooks_deserialize_schedule(bool &once, uint8_t **encoded_schedule,
                                uint8_t &error_code);
