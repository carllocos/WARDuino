#pragma once
#include <stddef.h>

#include "../Instrumentation/action.h"
#include "../Instrumentation/instrumentation.h"
#include "../WARDuino/structs.h"

#define MONITOR_ADDR_ERROR_CODE_REQUEST_HAS_WRONG_INTERRUPT_NR 1
#define MONITOR_ADDR_ERROR_CODE_REQUEST_HAS_UN_EXISTING_MOMENT 2
#define MONITOR_ADDR_ERROR_CODE_REQUEST_HAS_UNEXISTING_ADDR 3;

enum InstrumentMoment {
    InstrumentBefore = 0x01,
    InstrumentAfter = 0x02,
};

typedef struct {
    uint32_t addr{};
    InstrumentMoment moment{};
    Action *action{};
} MonitorAddrRequest;

typedef struct {
    uint8_t type{};
    uint8_t error_code{};
} MonitorAddrResponse;

void Interrupt_MonitorAddr_handle_request(const Channel &channel,
                                          const Module &module,
                                          InstrumentationManager &manager,
                                          Module *m, uint8_t *encoded_request);

bool Interrupt_MonitorAddr_deserialize_request(const Module &module,
                                               MonitorAddrRequest &request,
                                               uint8_t *encoded_request,
                                               uint8_t &error_code);

char *Interrupt_MonitorAddr_serialize_response(
    const MonitorAddrResponse &response, char *dest);

void Interrupt_MonitorAddr_send_response(const Channel &channel,
                                         const MonitorAddrResponse &response);