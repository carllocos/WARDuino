#pragma once
#include "../Utils/sockets.h"
#include "../WARDuino/structs.h"

#define INSPECT_ERROR_CODE_REQUEST_HAS_INVALID_INTERRUPT_NR 1
#define INSPECT_ERROR_CODE_REQUEST_HAS_INVALID_STATE_KIND 2

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

typedef struct {
    uint16_t numberOfInspects{};
    ExecutionState *requestedState{};
} InspectStateRequest;

typedef InspectStateRequest StateToInspect;

void Interrupt_Inspect_handle_request(const Channel &requester, Module *m,
                                      uint8_t *encoded_request);

bool Interrupt_Inspect_deserialize_request(InspectStateRequest &request,
                                           uint8_t *encoded_request,
                                           uint8_t &error_code);

void Interrupt_Inspect_inspect_json_output(const Channel &requester,
                                           const Module *m,
                                           const StateToInspect &state);