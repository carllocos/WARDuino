#pragma once
#include "../Instrumentation/hook.h"
#include "../WARDuino/structs.h"

enum HookMoment {
    InstrumentBefore = 0x01,
    InstrumentAfter = 0x02,
    InstrumentAround = 0x03
};

typedef struct HooksPrimitiveFunc {
    uint32_t func_idx;          // func for which the around hook is registered
    Primitive original_func{};  // original function
    Hook *hook{};               // hooks to perform instead of original_func
} HooksPrimitiveFunc;

typedef struct {
    uint32_t address{};         // wasm address that needs to be intercepted
    uint8_t original_opcode{};  // original opcode
    Hook *hook{};               // hooks to perform on address
} HooksWasmAddr;