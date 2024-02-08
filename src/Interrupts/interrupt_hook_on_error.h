#pragma once

#include "../Instrumentation/hook.h"
#include "../Instrumentation/instrumentation.h"

typedef struct {
    Hook *hook{};
} HookOnErrorRequest;

typedef struct {
    uint8_t response_type{};
    uint8_t error_code{};
} HookOnErrorResponse;

void Interrupt_HookOnError_handle_request(const Channel &requester,
                                          InstrumentationManager &manager,
                                          uint8_t *encoded_request);

bool Interrupt_HookOnError_deserialize_request(HookOnErrorRequest &dest,
                                               uint8_t *encoded_request,
                                               uint8_t &error_code);

ssize_t Interrupt_HookOnError_serialize_response(
    const HookOnErrorResponse &response, char *dest);

void Interrupt_HookOnError_send_response(const Channel &channel,
                                         const HookOnErrorResponse &response);