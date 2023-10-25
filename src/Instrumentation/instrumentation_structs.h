#pragma once
#include "../Instrumentation/action.h"
#include "../WARDuino/structs.h"

enum InstrumentMoment {
    InstrumentBefore = 0x01,
    InstrumentAfter = 0x02,
    InstrumentAround = 0x03
};

typedef struct InstrumentationPrimitiveFunc {
    uint32_t func_idx;  // func for which the around action is registered
    Primitive original_func{};  // original function
    Action *action{};           // actions to perform instead of original_func
} InstrumentationPrimitiveFunc;

typedef struct {
    uint32_t address{};         // wasm address that needs to be intercepted
    uint8_t original_opcode{};  // original opcode
    Action *action{};           // actions to perform on address
} InstrumentationWasmAddr;