#pragma once
#include "../Instrumentation/instrumentation.h"
#include "../WARDuino.h"

#define AROUND_FUNC_ERROR_CODE_NO_ERROR_CODE_SET 0
#define AROUND_FUNC_ERROR_CODE_UNEXISTING_AROUND_KIND 1
#define AROUND_FUNC_ERROR_CODE_UNEXISTING_LOCAL_FUNC 2
#define AROUND_FUNC_ERROR_CODE_NO_MEMORY_LEFT 3
#define AROUND_FUNC_ERROR_CODE_AROUND_ALREADY_EXISTS 4
#define AROUND_FUNC_ERROR_CODE_SCHEDULING_MODE_NOT_SUPPORTED 5
#define AROUND_FUNC_ERROR_CODE_SUBSTITUE_VALUE_IS_MALFORMED 6

typedef struct {
    uint8_t error_code{AROUND_FUNC_ERROR_CODE_NO_ERROR_CODE_SET};
    uint8_t type{};
} AroundFunctionResponse;

typedef struct {
    uint32_t func_idx;
    Action action;
} AroundFunctionRequest;

void Interrupt_AroundFunction_handle_request(const Channel &channel,
                                             InstrumentationManager &manager,
                                             Module *m,
                                             uint8_t *encoded_request);

void Interrupt_AroundFunction_send_response(
    const Channel &channel, const AroundFunctionResponse &response);

bool Interrupt_AroundFunction_deserialize_request(AroundFunctionRequest &dest,
                                                  uint8_t *encoded_data,
                                                  uint8_t &error_code);

ssize_t Interrupt_AroundFunction_serialize_response(
    const AroundFunctionResponse &response, char *dest);