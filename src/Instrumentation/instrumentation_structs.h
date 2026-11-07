#pragma once
#include "../Instrumentation/hook.h"
#include "../WARDuino/structs.h"

enum HookMoment { HookBefore = 0x01, HookAfter = 0x02, HookAround = 0x03 };

typedef struct HooksPrimitiveFunc {
    uint32_t func_idx;          // func for which the around hook is registered
    Primitive original_func{};  // original function
    Hook *hook{};               // hooks to perform instead of original_func
} HooksPrimitiveFunc;

/*
 * Flags of HooksWasmAddr
 */
// The shared pause hook (see `Hooks_shared_pause_hook`) runs after the other
// hooks. This avoids allocating a pause hook for each address on which the
// client wants to pause (e.g., to mutate state).
#define HOOKS_WASM_ADDR_PAUSE_AFTER_HOOKS 0x01
// The address has a single StateInspect hook, scheduled always, that inspects
// a shared state. Such hook is stored inline: `hooks.inline_id` holds its id.
// This is the common case for analyses (e.g., instruction coverage) and
// avoids allocating a hook per address.
#define HOOKS_WASM_ADDR_INLINE_INSPECT 0x02
// Only set with HOOKS_WASM_ADDR_INLINE_INSPECT: the inline hook inspects the
// shared pc + stack state instead of the shared pc state.
#define HOOKS_WASM_ADDR_INLINE_INSPECT_STACK 0x04

/*
 * Hooks registered on a wasm address. Kept at 16 bytes (64-bit targets) since
 * analyses may instrument every instruction of a module.
 */
typedef struct HooksWasmAddr {
    uint32_t address{};         // wasm address that needs to be intercepted
    uint8_t original_opcode{};  // original opcode
    uint8_t flags{};            // HOOKS_WASM_ADDR_* flags
    union {
        Hook *list;          // hooks to perform on address (when not inline)
        uint32_t inline_id;  // id of the inline StateInspect hook
    } hooks{};

    bool hasInlineInspect() const {
        return (this->flags & HOOKS_WASM_ADDR_INLINE_INSPECT) != 0;
    }

    bool pausesAfterHooks() const {
        return (this->flags & HOOKS_WASM_ADDR_PAUSE_AFTER_HOOKS) != 0;
    }

    Hook *hookList() const {
        return this->hasInlineInspect() ? nullptr : this->hooks.list;
    }

    bool isEmpty() const {
        return this->flags == 0 && this->hooks.list == nullptr;
    }
} HooksWasmAddr;

enum HookAroundKind { AroundInstruction = 0x00, AroundFunctionBody = 0x01 };

typedef struct HookAroundResultRequest {
    uint32_t id{};
    uint32_t addr{};
    HookAroundKind kind{};
    uint32_t nr_args{};
    bool has_result{};
    StackValue result{};
} HookAroundResultRequest;
