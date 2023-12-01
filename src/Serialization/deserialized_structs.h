#pragma once
#include "../Structs/struct_block.h"
#include "../Structs/struct_frame.h"
#include "../Structs/struct_stackvalue.h"
#include "../WARDuino/CallbackHandler.h"

typedef struct {
    uint32_t size;
    Block* functions;
} FunctionsDeserialized;

typedef struct {
    uint32_t size;
    Frame* callstack;
} FrameDeserialized;

typedef struct {
    uint32_t size;
    Event* events;
} EventsDeserialized;

typedef struct {
    uint32_t size;
    StackValue* stack;
} StackDeserialized;

typedef struct {
    uint32_t size;
    StackValue* globals;
} GlobalsDeserialized;

typedef struct {
    uint32_t size;
    uint8_t* bps;
} BreakpointDeserialized;
