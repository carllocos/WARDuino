#pragma once

#include "../Utils/sockets.h"
#include "../WARDuino/structs.h"

enum MonitorEventMoment { MonitorOnEventHandling = 0x01 };

typedef struct MonitorEventRequest {
    MonitorEventMoment moment{};
} MonitorEventRequest;

void Interrupt_Monitor_Event_handle_request(const Channel &requester, Module &m,
                                            uint8_t *encoded_request);

bool Interrupt_Monitor_Event_deserialize_request(MonitorEventRequest &dest,
                                                 uint8_t *encoded_request,
                                                 uint8_t &error_code);