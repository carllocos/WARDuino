#pragma once

#include "../Utils/util.h"
#include "../WARDuino/structs.h"

#define REMOTE_CALL_ERROR_CODE_OUT_OF_MEMORY 1

#define REMOTE_CALL_ERROR_CODE_READ_FROM_CLIENT 11
#define REMOTE_CALL_ERROR_CODE_WRITE_TO_CLIENT 11
#define REMOTE_CALL_ERROR_CODE_CLIENT_REPLY_TOO_LARGE 12
#define REMOTE_CALL_ERROR_CODE_CLIENT_CLOSED 13

#define REMOTE_CALL_ERROR_CODE_INVALID_FUNCTION 20
#define REMOTE_CALL_ERROR_CODE_MALFORMED_FUNCTION_ARGS 21
#define REMOTE_CALL_ERROR_CODE_INVALID_NUMBER_OF_ARGUMENTS 22

#define REMOTE_CALL_ERROR_CODE_MALFORMED_RESPONSE 30

typedef struct {
    bool success;
    StackValue *value{};
    char *exception_msg{};
} CallResult;

typedef struct {
    uint8_t type{};
    uint8_t error_code{};
    CallResult *result;
} FunCallResponse;

typedef struct {
    uint32_t fun{};
    uint32_t number_args{};
    StackValue *args{};
    bool isProxyCall{};
} FunCallRequest;

char *Interrupt_RemoteCall_serialize_request(FunCallRequest &request,
                                             size_t &serialized_size);

bool Interrupt_RemoteCall_deserialize_request(const Module *m,
                                              FunCallRequest &request,
                                              uint8_t *encoded_request,
                                              uint8_t &error_code);

bool Interrupt_RemoteCall_deserialize_response(FunCallResponse *response,
                                               uint8_t *encoded_response,
                                               const size_t size_response);

char *Interrupt_RemoteCall_serialize_response(const FunCallResponse &response,
                                              size_t &size_response);

void Interrupt_RemoteCall_free_response(FunCallResponse &response);

// Needed by the Target VM where the call has the happen
void Interrupt_RemoteCall_handle_request(const Channel &requester, Module *m,
                                         uint8_t *data);

void Interrupt_RemoteCall_do_local_call(Module *m, const uint32_t fun,
                                        StackValue *args, uint32_t nr_args,
                                        CallResult &result);

// Needed by the requester of the remote call
void Interrupt_RemoteCall_call(const uint32_t func, StackValue *args,
                               const uint32_t args_size, Channel &channel,
                               FunCallResponse *response, bool isProxyCall);
