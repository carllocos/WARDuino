#include "interrupt_hook_on_event.h"

#include "../Interrupts/interrupt_response.h"
#include "../Utils/macros.h"

#define RESPONSE_BUFFER_SIZE 100

void Interrupt_OnEventHook_handle_request(const Channel &requester, Module &m,
                                          uint8_t *encoded_request) {
    OnEventHookRequest request{};
    OnEventHookResponse response{};
    if (Interrupt_OnEventHook_deserialize_request(request, encoded_request,
                                                  response.error_code)) {
        response.type = INTERRUPT_RESPONSE_TYPE_SUCCESS;
    } else {
        response.type = INTERRUPT_RESPONSE_TYPE_ERROR;
    }
    Interrupt_OnEventHook_send_response(requester, response);
}

bool Interrupt_OnEventHook_deserialize_request(OnEventHookRequest &dest,
                                               uint8_t *encoded_request,
                                               uint8_t &error_code) {
    // format: interrupt nr | hook moment

    if (encoded_request[0] != interruptHookOnEvent) {
        error_code = ON_EVENT_HOOK_ERROR_CODE_INVALID_INTERRUPT_NR;
        return false;
    }

    dest.moment = (HookEventMoment)encoded_request[1];
    switch (dest.moment) {
        case HookOnEventHandling:
            break;
        default:
            error_code = ON_EVENT_HOOK_ERROR_CODE_INVALID_HOOK_MOMENT;
            return false;
    }
    return true;
}

void Interrupt_OnEventHook_send_response(const Channel &output,
                                         const OnEventHookResponse &response) {
    char serialization[RESPONSE_BUFFER_SIZE];
    if (Interrupt_OnEventHook_serialize_response(response, serialization,
                                                 RESPONSE_BUFFER_SIZE)) {
        output.write("%s\n", serialization);
    }
}

bool Interrupt_OnEventHook_serialize_response(
    const OnEventHookResponse &response, char *dest, uint32_t dest_size) {
    return Interrupt_OnEventHook_serialize_json_response(response, dest,
                                                         dest_size);
}

bool Interrupt_OnEventHook_serialize_json_response(
    const OnEventHookResponse &response, char *dest, uint32_t dest_size) {
    FATAL("TODO");
    return false;
}