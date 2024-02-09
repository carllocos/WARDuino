#include "interrupt_hook_on_error.h"

#include "../Interrupts/interrupt_response.h"

bool addHook(InstrumentationManager &manager, Hook &hook, uint8_t &error_code);

void Interrupt_HookOnError_handle_request(const Channel &requester,
                                          InstrumentationManager &manager,
                                          uint8_t *encoded_request) {
    HookOnErrorRequest request{};
    HookOnErrorResponse response{};
    Hook hook;
    request.hook = &hook;

    if (Interrupt_HookOnError_deserialize_request(request, encoded_request,
                                                  response.error_code) &&
        addHook(manager, *request.hook, response.error_code)) {
        response.type = INTERRUPT_RESPONSE_TYPE_SUCCESS;
    } else {
        response.type = INTERRUPT_RESPONSE_TYPE_ERROR;
    }

    Interrupt_HookOnError_send_response(requester, response);
}

bool Interrupt_HookOnError_deserialize_request(HookOnErrorRequest &dest,
                                               uint8_t *encoded_request,
                                               uint8_t &error_code) {
    // format: interrupt nr | Hook
    if (*encoded_request++ != interruptHookOnError) {
        error_code = HOOK_ON_ERROR_ERROR_CODE_INVALID_INTERRUPT_NR;
        return false;
    }
    return Hooks_deserialize_hook(*dest.hook, &encoded_request, error_code);
}

void Interrupt_HookOnError_send_response(const Channel &channel,
                                         const HookOnErrorResponse &response) {
    char serialization[10];
    if (Interrupt_HookOnError_serialize_response(response, serialization) > 0) {
        channel.write("%s\n", serialization);
    }
}

ssize_t Interrupt_HookOnError_serialize_response(
    const HookOnErrorResponse &response, char *dest) {
    return Interrupt_serialize_hexa_string_response(interruptHookOnError,
                                                    response.type, dest);
}

bool addHook(InstrumentationManager &manager, Hook &hook, uint8_t &error_code) {
    if (!manager.addHookOnError(hook)) {
        error_code = HOOK_ON_ERROR_ERROR_CODE_UNALLOWED_HOOK;
        return false;
    }
    return true;
}

void Interrupt_HookOnError_send_JSON_subscribe_message(
    const Channel &ouput, std::function<void()> hookOutput) {
    Interrupt_send_JSON_subscribe_message(ouput, interruptHookOnError,
                                          hookOutput);
}