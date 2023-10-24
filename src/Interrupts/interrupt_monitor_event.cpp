#include "interrupt_monitor_event.h"

#include "../Utils/macros.h"

void Interrupt_Monitor_Event_handle_request(const Channel &requester, Module &m,
                                            uint8_t *encoded_request) {
    FATAL("TODO");
}

bool Interrupt_Monitor_Event_deserialize_request(MonitorEventRequest &dest,
                                                 uint8_t *encoded_request,
                                                 uint8_t &error_code) {
    FATAL("TODO");
}