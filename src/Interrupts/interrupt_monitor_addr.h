#pragma once
#include <stddef.h>

#include <functional>

#include "../Instrumentation/action.h"
#include "../Instrumentation/instrumentation.h"
#include "../WARDuino/structs.h"

#define MONITOR_ADDR_ERROR_CODE_REQUEST_HAS_WRONG_INTERRUPT_NR 1
#define MONITOR_ADDR_ERROR_CODE_REQUEST_HAS_UN_EXISTING_MOMENT 2
#define MONITOR_ADDR_ERROR_CODE_REQUEST_HAS_UNEXISTING_ADDR 3;
#define MONITOR_ADDR_ERROR_CODE_COULD_NOT_ADD_ACTION 4;

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
                                          Module &module,
                                          InstrumentationManager &manager,
                                          uint8_t *encoded_request);

bool Interrupt_MonitorAddr_deserialize_request(MonitorAddrRequest &request,
                                               uint8_t *encoded_request,
                                               uint8_t &error_code);

ssize_t Interrupt_MonitorAddr_serialize_response(
    const MonitorAddrResponse &response, char *dest);

void Interrupt_MonitorAddr_send_response(const Channel &channel,
                                         const MonitorAddrResponse &response);

void Interrupt_MonitorAddr_send_JSON_subscribe_message(
    const Channel &output, InstrumentMoment moment, uint32_t addr,
    std::function<void()> actionOutput);