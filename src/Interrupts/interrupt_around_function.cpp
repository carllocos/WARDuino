#include "interrupt_around_function.h"

#include "../Instrumentation/instrumentation.h"
#include "../Interrupts/interrupts.h"
#include "../Utils/macros.h"
#include "../Utils/util.h"

/*
 * Declaration private functions
 */
bool registerAroundFunctionAction(InstrumentationManager &manager, Module &m,
                                  const AroundFunctionRequest &request,
                                  uint8_t &error_code);

bool deserialize_scheduling(AroundFunctionRequest &dest,
                            uint8_t **encoded_schedule, uint8_t &error_code);

bool deserialize_action(AroundFunctionRequest &dest, uint8_t **data,
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
    return deserialize_scheduling(dest, &data, error_code) &&
           deserialize_action(dest, &data, error_code);
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

bool deserialize_scheduling(AroundFunctionRequest &dest,
                            uint8_t **encoded_schedule, uint8_t &error_code) {
    // format expected: SCHEDULE_KIND (1 BYTE)
    // format timestamp: nr of instructions (LEB32) | nr of events (LEB32);

    ScheduleKind schedule = (ScheduleKind) * *encoded_schedule;
    *encoded_schedule += 1;
    switch (schedule) {
        case ScheduleOnce:
        case ScheduleAlways:
            break;
        case ScheduleOnTimeStamp:
        case ScheduleBeforeTimeStamp:
        case ScheduleAfterTimeStamp:
            dest.action.schedule.value.timeStamp.nr_of_instructions =
                read_LEB_32(encoded_schedule);
            dest.action.schedule.value.timeStamp.nr_of_events =
                read_LEB_32(encoded_schedule);
            break;
        default:
            error_code = AROUND_FUNC_ERROR_CODE_SCHEDULING_MODE_NOT_SUPPORTED;
            return false;
    }
    dest.action.schedule.kind = schedule;
    return true;
}

bool deserialize_action(AroundFunctionRequest &dest, uint8_t **encoded_action,
                        uint8_t &error_code) {
    // format expected: ActionKind (1 BYTE)
    // RemoteCall: target fidx (LEB32)
    // ValueSubstitution: hasValue (1 byte) | value;
    ActionKind kind = (ActionKind) * *encoded_action;
    *encoded_action += 1;
    switch (kind) {
        case RemoteCall:
            dest.action.value.target_fidx = read_LEB_32(encoded_action);
            break;
        case ValueSubstitution: {
            bool hasValue = **encoded_action;
            *encoded_action += 1;
            if (hasValue) {
                ValueSerializationConfig config;
                config.includeType = true;
                config.includeIndex = false;
                size_t bytes_read = deserializeStackValue(
                    dest.action.value.result, config, *encoded_action);
                if (bytes_read <= 0) {
                    error_code =
                        AROUND_FUNC_ERROR_CODE_SUBSTITUE_VALUE_IS_MALFORMED;
                    return false;
                }
            } else {
                dest.action.value.result = nullptr;
            }
            break;
        }
        default:
            printf("ActionKind %02X is not supported\n", kind);
            error_code = AROUND_FUNC_ERROR_CODE_UNEXISTING_AROUND_KIND;
            return false;
    }
    dest.action.kind = kind;
    return true;
}