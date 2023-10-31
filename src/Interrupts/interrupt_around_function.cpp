#include "interrupt_around_function.h"

#include "../Interrupts/interrupt_response.h"
#include "../Utils/macros.h"
#include "../Utils/util.h"

/*
 * Declaration private functions
 */
bool registerAroundFunctionHook(InstrumentationManager &manager, Module &m,
                                const AroundFunctionRequest &request,
                                uint8_t &error_code);

/*
 * Public functions
 */

ssize_t Interrupt_AroundFunction_serialize_response(
    const AroundFunctionResponse &response, char *dest) {
    return Interrupt_serialize_JSON_response(
        interruptAroundFunction, response.type, response.error_code, dest);
}

void Interrupt_AroundFunction_send_response(
    const Channel &channel, const AroundFunctionResponse &response) {
    char buffer[100]{};
    if (Interrupt_AroundFunction_serialize_response(response, buffer) > 0) {
        channel.write(buffer);
    }
}

void Interrupt_AroundFunction_handle_request(const Channel &channel,
                                             InstrumentationManager &manager,
                                             Module *m,
                                             uint8_t *encoded_request) {
    AroundFunctionRequest request;
    StackValue val;
    request.hook.value.result = &val;  // trick to avoid malloc

    AroundFunctionResponse response;
    uint8_t error{};

    if (Interrupt_AroundFunction_deserialize_request(request, encoded_request,
                                                     error) &&
        registerAroundFunctionHook(manager, *m, request, error)) {
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
    // format: Target func (LEB32) | Schedule | Hook
    uint8_t *data = encoded_data;
    dest.func_idx = read_LEB_32(&data);
    return Hooks_deserialize_hook(dest.hook, &data, error_code);
}

/*
 * Private functions
 */

bool registerAroundFunctionHook(InstrumentationManager &manager, Module &m,
                                const AroundFunctionRequest &request,
                                uint8_t &error_code) {
    if (request.func_idx >= m.function_count) {
        error_code = AROUND_FUNC_ERROR_CODE_UNEXISTING_LOCAL_FUNC;
        return false;
    }
    if (!manager.isAddHookAllowed(request.func_idx)) {
        error_code = AROUND_FUNC_ERROR_CODE_AROUND_ALREADY_EXISTS;
        return false;
    }

    if (!manager.addAroundFunctionHook(m, request.func_idx, request.hook)) {
        error_code =
            AROUND_FUNC_ERROR_CODE_NO_MEMORY_LEFT;  // TODO change error_code
        return false;
    }

    return true;
}