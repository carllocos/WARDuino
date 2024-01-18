#pragma once

#include "../Utils/sockets.h"
#include "../WARDuino/structs.h"

#define ON_EVENT_HOOK_ERROR_CODE_INVALID_INTERRUPT_NR 1;
#define ON_EVENT_HOOK_ERROR_CODE_INVALID_HOOK_MOMENT 2;

enum HookEventMoment { HookOnEventHandling = 0x01 };

typedef struct OnEventHookRequest {
    HookEventMoment moment{};
} OnEventHookRequest;

typedef struct OnEventHookResponse {
    uint8_t type{};
    uint8_t error_code{};
} OnEventHookResponse;

void Interrupt_OnEventHook_handle_request(const Channel &requester, Module &m,
                                          uint8_t *encoded_request);

bool Interrupt_OnEventHook_deserialize_request(OnEventHookRequest &dest,
                                               uint8_t *encoded_request,
                                               uint8_t &error_code);

void Interrupt_OnEventHook_send_response(const Channel &output,
                                         const OnEventHookResponse &response);

bool Interrupt_OnEventHook_serialize_response(
    const OnEventHookResponse &response, char *dest, uint32_t dest_size);

bool Interrupt_OnEventHook_serialize_json_response(
    const OnEventHookResponse &response, char *dest, uint32_t dest_size);