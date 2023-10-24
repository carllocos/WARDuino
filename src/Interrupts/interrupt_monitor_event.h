#pragma once

#include "../Utils/sockets.h"
#include "../WARDuino/structs.h"

enum MonitorEventMoment { MonitorOnEventHandling = 0x01 };

typedef struct MonitorEventRequest {
    MonitorEventMoment moment{};
} MonitorEventRequest;

typedef struct MonitorEventResponse {
    uint8_t type{};
    uint8_t error_code{};
} MonitorEventResponse;

void Interrupt_Monitor_Event_handle_request(const Channel &requester, Module &m,
                                            uint8_t *encoded_request);

bool Interrupt_Monitor_Event_deserialize_request(MonitorEventRequest &dest,
                                                 uint8_t *encoded_request,
                                                 uint8_t &error_code);

void Interrupt_MonitorEvent_send_response(const Channel &output,
                                          const MonitorEventResponse &response);

bool Interrupt_MonitorEvent_serialize_response(
    const MonitorEventResponse &response, char *dest, uint32_t dest_size);

bool Interrupt_MonitorEvent_serialize_json_response(
    const MonitorEventResponse &response, char *dest, uint32_t dest_size);