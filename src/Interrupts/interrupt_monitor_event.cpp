#include "interrupt_monitor_event.h"

#include "../Interrupts/interrupt_response.h"
#include "../Utils/macros.h"

void Interrupt_Monitor_Event_handle_request(const Channel &requester, Module &m,
                                            uint8_t *encoded_request) {
    MonitorEventRequest request{};
    MonitorEventResponse response{};
    if (Interrupt_Monitor_Event_deserialize_request(request, encoded_request,
                                                    response.error_code)) {
        response.type = INTERRUPT_RESPONSE_TYPE_SUCCESS;
    } else {
        response.type = INTERRUPT_RESPONSE_TYPE_ERROR;
    }
    Interrupt_MonitorEvent_send_response(requester, response);
}

bool Interrupt_Monitor_Event_deserialize_request(MonitorEventRequest &dest,
                                                 uint8_t *encoded_request,
                                                 uint8_t &error_code) {
    FATAL("TODO");
}