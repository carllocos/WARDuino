#include "interrupt_around_function.h"

#include "../Instrumentation/instrumentation.h"
#include "../Interrupts/interrupts.h"
#include "../Utils/macros.h"
#include "../Utils/util.h"

bool registerAroundFunction(InstrumentationManager &manager, Module &m,
                            const AroundFunctionRequest &request,
                            uint8_t &error_code);

ssize_t Interrupt_AroundFunction_serialize_response(
    const AroundFunctionResponse &response, char *dest) {
    ssize_t offset = 0;
    offset += sprintf(dest, R"({"interrupt":"%02X","kind":"%02X")",
                      interruptAroundFunction, response.type);
    if (response.type == INTERRUPT_RESPONSE_TYPE_ERROR) {
        offset += sprintf(dest + offset, R"(,"error_code":"%02X")",
                          interruptAroundFunction, response.type);
    }
    offset += sprintf(dest + offset, "}\n");
    return offset;
}

void Interrupt_AroundFunction_send_response(
    const Channel &channel, const AroundFunctionResponse &response) {
    char buffer[100]{};
    Interrupt_AroundFunction_serialize_response(response, buffer);
    channel.write(buffer);
}

void Interrupt_AroundFunction_handle_request(const Channel &channel,
                                             InstrumentationManager &manager,
                                             Module *m,
                                             uint8_t *encoded_request) {
    AroundFunctionRequest request;
    AroundFunctionResponse response;
    uint8_t error;

    if (Interrupt_AroundFunction_deserialize_request(request, encoded_request,
                                                     error) &&
        registerAroundFunction(manager, *m, request, error)) {
        response.type = INTERRUPT_RESPONSE_TYPE_SUCCESS;
    } else {
        response.type = INTERRUPT_RESPONSE_TYPE_ERROR;
        response.error_code = error;
    }

    Interrupt_AroundFunction_send_response(channel, response);
}

bool Interrupt_AroundFunction_deserialize_request(AroundFunctionRequest &dest,
                                                  uint8_t *encoded_data,
                                                  uint8_t &error_code) {
    uint8_t *data = encoded_data;
    dest.kind = (AroundKind)*data++;
    dest.func_idx = read_LEB_32(&data);
    switch (dest.kind) {
        case RemoteCall:
            dest.action.target_fidx = read_LEB_32(&data);
            break;
        case ProxyCall:
        case UseResult:
        case NativeCall:
            printf("AroundFunction: around kind %02X is not yet supported\n",
                   dest.kind);
        default:
            error_code = AROUND_FUNC_ERROR_CODE_UNEXISTING_AROUND_KIND;
            return false;
    }
    return true;
}

/*
 * Private functions
 */

bool registerAroundFunction(InstrumentationManager &manager, Module &m,
                            const AroundFunctionRequest &request,
                            uint8_t &error_code) {
    if (request.func_idx >= m.function_count) {
        error_code = AROUND_FUNC_ERROR_CODE_UNEXISTING_LOCAL_FUNC;
        return false;
    }
    if (manager.has_AroundFunction(request.func_idx)) {
        error_code = AROUND_FUNC_ERROR_CODE_AROUND_ALREADY_EXISTS;
        return false;
    }

    AroundFunction *around = manager.new_AroundFunction();
    if (around == nullptr) {
        error_code = AROUND_FUNC_ERROR_CODE_NO_MEMORY_LEFT;
        return false;
    }

    around->func_idx = request.func_idx;
    around->kind = request.kind;
    around->action.target_fidx = request.action.target_fidx;

    if (!manager.add_AroundFunction(m, around)) {
        error_code = AROUND_FUNC_ERROR_CODE_NO_MEMORY_LEFT;
        manager.free_AroundFunction(around);
        return false;
    }

    return true;
}