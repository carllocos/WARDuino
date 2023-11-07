#include "interrupt_monitor_event.h"

#include "../Interrupts/interrupt_response.h"
#include "../Utils/macros.h"

#define RESPONSE_BUFFER_SIZE 100

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
    // format: interrupt nr | monitor moment

    if (encoded_request[0] != interruptMonitorEvent) {
        error_code = MONITOR_EVENT_ERROR_CODE_INVALID_INTERRUPT_NR;
        return false;
    }

    dest.moment = (MonitorEventMoment)encoded_request[1];
    switch (dest.moment) {
        case MonitorOnEventHandling:
            break;
        default:
            error_code = MONITOR_EVENT_ERROR_CODE_INVALID_MONITOR_MOMENT;
            return false;
    }
    return true;
}

void Interrupt_MonitorEvent_send_response(
    const Channel &output, const MonitorEventResponse &response) {
    char serialization[RESPONSE_BUFFER_SIZE];
    if (Interrupt_MonitorEvent_serialize_response(response, serialization,
                                                  RESPONSE_BUFFER_SIZE)) {
        output.write("%s\n", serialization);
    }
}

bool Interrupt_MonitorEvent_serialize_response(
    const MonitorEventResponse &response, char *dest, uint32_t dest_size) {
    return Interrupt_MonitorEvent_serialize_json_response(response, dest,
                                                          dest_size);
}

bool Interrupt_MonitorEvent_serialize_json_response(
    const MonitorEventResponse &response, char *dest, uint32_t dest_size) {
    FATAL("TODO");
    return false;
}