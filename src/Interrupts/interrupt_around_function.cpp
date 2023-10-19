#include "interrupt_around_function.h"

#include "../Interrupts/interrupt_response.h"
#include "../Utils/macros.h"
#include "../Utils/util.h"

/*
 * Declaration private functions
 */
bool registerAroundFunctionAction(InstrumentationManager &manager, Module &m,
                                  const AroundFunctionRequest &request,
                                  uint8_t &error_code);

/*
 * Public functions
 */

ssize_t Interrupt_AroundFunction_serialize_response(
    const AroundFunctionResponse &response, char *dest) {
    ssize_t offset = 0;
    offset += sprintf(dest, R"({"interrupt":"%02X","kind":"%02X")",
                      interruptAroundFunction, response.type);
    if (response.type == INTERRUPT_RESPONSE_TYPE_ERROR) {
        offset += sprintf(dest + offset, R"(,"error_code":"%02X")",
                          response.error_code);
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
    StackValue val;
    request.action.value.result = &val;  // trick to avoid malloc

    AroundFunctionResponse response;
    uint8_t error{};

    if (Interrupt_AroundFunction_deserialize_request(request, encoded_request,
                                                     error) &&
        registerAroundFunctionAction(manager, *m, request, error)) {
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
    // format: Target func (LEB32) | Schedule | Action
    uint8_t *data = encoded_data;
    dest.func_idx = read_LEB_32(&data);
    return Actions_deserialize_action(dest.action, &data, error_code);
}

/*
 * Private functions
 */

bool registerAroundFunctionAction(InstrumentationManager &manager, Module &m,
                                  const AroundFunctionRequest &request,
                                  uint8_t &error_code) {
    if (request.func_idx >= m.function_count) {
        error_code = AROUND_FUNC_ERROR_CODE_UNEXISTING_LOCAL_FUNC;
        return false;
    }
    if (!manager.isAddActionAllowed(request.func_idx)) {
        error_code = AROUND_FUNC_ERROR_CODE_AROUND_ALREADY_EXISTS;
        return false;
    }

    if (!manager.addAroundFunctionAction(m, request.func_idx, request.action)) {
        error_code =
            AROUND_FUNC_ERROR_CODE_NO_MEMORY_LEFT;  // TODO change error_code
        return false;
    }

    return true;
}