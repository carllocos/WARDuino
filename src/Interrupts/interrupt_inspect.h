#pragma once
#include "../Utils/sockets.h"
#include "../WARDuino/structs.h"

enum ExecutionState {
    pcState = 0x01,
    breakpointsState = 0x02,
    callstackState = 0x03,
    globalsState = 0x04,
    tableState = 0x05,
    memoryState = 0x06,
    branchingTableState = 0x07,
    stackState = 0x08,
    callbacksState = 0x09,
    eventsState = 0x0A
};

void Interrupt_Inspect_handle_request(const Channel &requester, Module *m,
                                      uint8_t *data);

void Interrupt_Inspect_inspect_json_output(const Channel &requester,
                                           const Module *m,
                                           uint16_t sizeStateArray,
                                           const ExecutionState *state);