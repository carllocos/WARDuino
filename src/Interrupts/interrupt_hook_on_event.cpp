#include "interrupt_hook_on_event.h"

#include "../Interrupts/interrupt_response.h"
#include "../Utils/macros.h"
#include "../Utils/util.h"

#define RESPONSE_BUFFER_SIZE 10

void Interrupt_HookOnEvent_handle_request(const Channel &requester,
                                          InstrumentationManager &manager,
                                          uint8_t *encoded_request) {
    OnEventHookRequest request{};
    OnEventHookResponse response{};
    Hook hook;
    request.hook = &hook;

    if (Interrupt_HookOnEvent_deserialize_request(request, encoded_request,
                                                  response.error_code)) {
        if (manager.isAddHookOnEventAllowed(*request.hook)) {
            manager.addHookOnNewEvent(hook);
            response.type = INTERRUPT_RESPONSE_TYPE_SUCCESS;
        } else {
            response.error_code = HOOK_ON_EVENT_ERROR_CODE_UNALLOWED_HOOK;
            response.type = INTERRUPT_RESPONSE_TYPE_ERROR;
        }
    } else {
        response.type = INTERRUPT_RESPONSE_TYPE_ERROR;
    }

    Interrupt_HookOnEvent_send_response(requester, response);
}

bool Interrupt_HookOnEvent_deserialize_request(OnEventHookRequest &dest,
                                               uint8_t *encoded_request,
                                               uint8_t &error_code) {
    // format: interrupt nr | hook moment | Hook

    if (*encoded_request++ != interruptHookOnEvent) {
        error_code = ON_EVENT_HOOK_ERROR_CODE_INVALID_INTERRUPT_NR;
        return false;
    }
    dest.moment = (HookEventMoment)*encoded_request++;
    switch (dest.moment) {
        case HookOnNewEvent:
        case HookOnEventHandling:
        case HookAfterEventHandled:
            break;
        default:
            error_code = ON_EVENT_HOOK_ERROR_CODE_INVALID_HOOK_MOMENT;
            return false;
    }
    return Hooks_deserialize_hook(*dest.hook, &encoded_request, error_code);
}

void Interrupt_HookOnEvent_send_response(const Channel &output,
                                         const OnEventHookResponse &response) {
    char serialization[RESPONSE_BUFFER_SIZE];
    if (Interrupt_OnEventHook_serialize_response(response, serialization)) {
        output.write("%s\n", serialization);
    }
}

bool Interrupt_OnEventHook_serialize_response(
    const OnEventHookResponse &response, char *dest) {
    return Interrupt_HookOnEvent_serialize_hexa_string_response(response, dest);
}

bool Interrupt_HookOnEvent_serialize_hexa_string_response(
    const OnEventHookResponse &response, char *dest) {
    return Interrupt_serialize_hexa_string_response(interruptHookOnEvent,
                                                    response.type, dest) > -1;
}

void Interrupt_HookOnEvent_send_JSON_subscribe_message(
    const Channel &output, HookEventMoment moment,
    std::function<void()> hookOutput) {
    auto subscriptionMsgBody = [&output, moment, hookOutput]() {
        output.write(R"({"moment":"%02X","val":)", moment);
        hookOutput();
        output.write("}");
    };
    Interrupt_send_JSON_subscribe_message(output, interruptHookOnEvent,
                                          subscriptionMsgBody);
}

void Interrupt_HookOnEvent_send_Binary_subscribe_message(const Channel &output,
                                                         const Event &ev) {
    // format: interrupt_nr (1byte) | message_type (1byte) | event
    // event: topic | payload
    // topic: size topic (LEB32) | topic (LEB32)
    // payload: size payload (LEB32) | payload (LEB32)

    // calculate total size buffer:
    size_t encodingSize =
        1 + 1 + size_for_64BIT_LEB(ev.topic.size()) + ev.topic.size() +
        size_for_64BIT_LEB(ev.payload.size()) + ev.payload.size();
    uint8_t *buffer = (uint8_t *)malloc(encodingSize);
    if (buffer == nullptr) {
        return;
    }

    // write interrupt nr
    buffer[0] = interruptHookOnEvent;
    buffer[1] = INTERRUPT_RESPONSE_TYPE_SUBSCRIPTION;

    // TODO refactor the following event encoding
    // write Topic size and content
    size_t offset = 2;
    offset += write_64BIT_LEB(ev.topic.size(), buffer + offset);
    std::memcpy(buffer + offset, ev.topic.c_str(), ev.topic.size());
    offset += ev.topic.size();

    // write payload size and content
    offset += write_64BIT_LEB(ev.payload.size(), buffer + offset);
    std::memcpy(buffer + offset, ev.payload.c_str(), ev.payload.size());

    HexUInt8Encoding dest{};
    if (uint8_to_hex(buffer, encodingSize, &dest)) {
        output.write("%s\n", dest.encoding);
        free(dest.encoding);
    }
    free(buffer);
}