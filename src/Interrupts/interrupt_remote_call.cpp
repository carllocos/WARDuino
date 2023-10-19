#include "interrupt_remote_call.h"

#include "../Interrupts/interrupt_response.h"
#include "../Utils/macros.h"
#include "../WARDuino/vm_exception.h"

class ChannelReader {
   private:
    Channel &channel;
    uint32_t buffer_size = 0;
    uint32_t buffer_max_size = 100;
    char buffer[100]{};
    std::deque<std::string> lines;

    int readUntilChar(char deliminator) {
        ssize_t readAmount{};
        size_t offset = 0;
        bool found_until_char = false;
        while ((readAmount = this->channel.read(
                    buffer + offset, buffer_max_size - buffer_size)) != -1) {
            // Split buffer into strings via deliminator
            for (int i = buffer_size; i < buffer_size + readAmount; ++i) {
                if (buffer[i] == deliminator) {
                    std::string content(buffer + offset, buffer + i);
                    lines.push_back(content);
                    offset = i + 1;
                    found_until_char = true;
                }
            }

            // move remaining buffer content to start buffer
            int idx = 0;
            while (offset < buffer_size + readAmount) {
                buffer[idx++] = buffer[offset++];
            }
            buffer[idx] = '\0';
            buffer_size = idx;
            if (found_until_char) {
                break;
            }
        }

        if (readAmount == -1) {
            return -1;
        }

        return offset;
    }

   public:
    ChannelReader(Channel &t_channel) : channel(t_channel) {}

    int readUntilChar(std::string &dest, char deliminator) {
        int bytes_read = -1;
        if (this->lines.empty()) {
            bytes_read = this->readUntilChar(deliminator);
        }
        if (this->lines.empty()) {
            return bytes_read;
        }
        dest = this->lines.front();
        this->lines.pop_front();
        return dest.length();
    }

    int readLine(std::string &line) { return this->readUntilChar(line, '\n'); }
};

void sendResponse(const Channel &channel, const FunCallResponse &response) {
    size_t serialize_size;
    char *encoding =
        Interrupt_RemoteCall_serialize_response(response, serialize_size);
    if (encoding != nullptr) {
        channel.write("%s\n", encoding);
        free(encoding);
    }
}

void Interrupt_RemoteCall_do_local_call(Module *m, const uint32_t fun,
                                        StackValue *args, uint32_t nr_args,
                                        CallResult &result) {
    WARDuino *instance = WARDuino::instance();
    RunningState current = instance->program_state;
    instance->program_state = WARDUINOrun;
    result.success = WARDuino::instance()->invoke(m, fun, nr_args, args);
    if (result.success) {
        printf("TODO: Find a good way to determine if call returns result\n");
        if (m->sp >= 0) {
            *result.value = m->stack[m->sp];
        } else {
            result.value = nullptr;
        }
    } else {
        result.exception_msg = strdup(VM_Exception_get_exception());
        result.value = nullptr;
    }
    instance->program_state = current;
}

void Interrupt_RemoteCall_handle_request(const Channel &requester, Module *m,
                                         uint8_t *data) {
    FunCallRequest request{};
    FunCallResponse response{};
    StackValue resultValue{};
    CallResult result{};
    uint8_t error{};

    result.value = &resultValue;  // trick to avoid malloc
    response.result = &result;

    if (Interrupt_RemoteCall_deserialize_request(m, request, data, error)) {
        Interrupt_RemoteCall_do_local_call(m, request.fun, request.args,
                                           request.number_args, result);
        response.type = INTERRUPT_RESPONSE_TYPE_SUCCESS;
    } else {
        response.type = INTERRUPT_RESPONSE_TYPE_ERROR;
        response.error_code = error;
    }

    sendResponse(requester, response);
}

char *Interrupt_RemoteCall_serialize_request(FunCallRequest &request,
                                             size_t &serialized_size) {
    // format: interrupt | LEB32 funcid | Stack | newline | null termination

    const ValueSerializationConfig config{
        .includeIndex = false,
        .includeType = false,
    };

    // serialisation
    size_t total_size =
        1 + size_for_LEB(request.fun) +
        size_for_stackvalues(request.args, request.number_args, config) + 2;
    uint8_t *buffer = (uint8_t *)malloc(total_size);

    // add interruptnr
    buffer[0] = interruptFunCall;
    size_t offset = 1;

    // add LEB128 Func ID
    offset += write_LEB(request.fun, buffer + offset);

    // serialize args
    offset += serializeStackValues(request.args, request.number_args, config,
                                   buffer + offset);

    buffer[offset++] = '\n';
    buffer[offset++] = '\0';
    char *hex = uint8_to_hex(buffer, offset);
    free(buffer);

    serialized_size = hex == nullptr ? 0 : offset * 2;
    return hex;
}

bool Interrupt_RemoteCall_deserialize_request(const Module *m,
                                              FunCallRequest &request,
                                              uint8_t *encoded_request,
                                              uint8_t &error_code) {
    // format: LEB32 funcid | LEB32 nr args | StackValues | newline
    request.fun = read_LEB_32(&encoded_request);
    if (request.fun > m->function_count) {
        error_code = REMOTE_CALL_ERROR_CODE_INVALID_FUNCTION;
        return false;
    }

    Type *type = m->functions[request.fun].type;
    if (type->param_count == 0) {
        request.number_args = 0;
        request.args = nullptr;
        return true;
    }

    uint8_t *data = encoded_request;
    request.number_args = read_LEB_32(&data);
    if (request.number_args != type->param_count) {
        error_code = REMOTE_CALL_ERROR_CODE_INVALID_NUMBER_OF_ARGUMENTS;
        return false;
    }

    ValueSerializationConfig config{.includeIndex = false,
                                    .includeType = false};
    request.args = deserializeStackValues(encoded_request, config, type);
    if (request.args == nullptr) {
        error_code = REMOTE_CALL_ERROR_CODE_MALFORMED_FUNCTION_ARGS;
        return false;
    }

    return true;
}

uint8_t *serialize_success_response(const FunCallResponse &response,
                                    size_t &size_encoding) {
    // format header: interrupt (1byte) | message_type (1byte) | success (1byte)
    // format body 1: has_value (1 byte) | StackValue (optional) OR format body
    // 2: has_excp_msg (1 byte) | excp_msg (optional) end: newline

    ValueSerializationConfig config{};
    config.includeType = true;
    config.includeIndex = false;

    size_encoding = 6;  // 1 for interruptnr, 1 for message_type, 1 for success,
                        // 1 for has_value or has_expc_msg, 1 for newline, 1 for
                        // null termination
    if (response.result->success && response.result->value != nullptr) {
        size_encoding +=
            serializeStackValueSize(response.result->value, config);
    } else if (!response.result->success &&
               response.result->exception_msg != nullptr) {
        size_encoding += strlen(response.result->exception_msg);
    }

    uint8_t *encoded_response = (uint8_t *)malloc(size_encoding);
    encoded_response[0] = interruptFunCall;
    encoded_response[1] = response.type;
    encoded_response[2] = response.result->success;
    encoded_response[3] = response.result->success
                              ? response.result->value != nullptr
                              : response.result->exception_msg != nullptr;
    encoded_response[size_encoding - 2] = '\n';
    encoded_response[size_encoding - 1] = '\0';

    if (response.result->value != nullptr) {
        serializeStackValue(*response.result->value, config,
                            encoded_response + 4);
    } else if (response.result->exception_msg != nullptr) {
        size_t str_len = size_encoding - 3;
        memcpy(encoded_response + 3, response.result->exception_msg, str_len);
    }
    return encoded_response;
}

uint8_t *serialize_error_response(const FunCallResponse &response,
                                  size_t &size_response) {
    // format: interrupt nr (1byte) | message_type (1byte) | error_code (1byte)
    // | newline | null termination
    size_response = 5;
    uint8_t *encoded_response = (uint8_t *)malloc(size_response);
    encoded_response[0] = interruptFunCall;
    encoded_response[1] = INTERRUPT_RESPONSE_TYPE_ERROR;
    encoded_response[2] = response.error_code;
    encoded_response[3] = '\n';
    encoded_response[4] = '\0';
    return encoded_response;
}

char *Interrupt_RemoteCall_serialize_response(const FunCallResponse &response,
                                              size_t &size_response) {
    size_t size_encoding{};
    uint8_t *encoding =
        response.type == INTERRUPT_RESPONSE_TYPE_SUCCESS
            ? serialize_success_response(response, size_encoding)
            : serialize_error_response(response, size_encoding);

    char *hexa_encoding = nullptr;
    if (encoding != nullptr) {
        hexa_encoding = uint8_to_hex(encoding, size_encoding);
        size_response = size_encoding * 2;  // size in hexa
        free(encoding);
    }
    return hexa_encoding;
}

uint8_t char_to_uint8(char c) {
    switch (c) {
        case '0' ... '9':
            return c - '0';
        case 'A' ... 'F':
            return c - 'A' + 10;
        case 'a' ... 'f':
            return c - 'e' + 10;
    }
    FATAL("Func should not be used with a non char\n");
    return 0;
}

bool valid_hex_char(char c) {
    switch (c) {
        case '0' ... '9':
        case 'A' ... 'F':
        case 'a' ... 'f':
            return true;
        default:
            return false;
    }
}

uint8_t *hex_to_uint8_t(char *hexString) {
    size_t hexStringLength = strlen(hexString);

    size_t number_hex_chars = 0;
    for (size_t i = 0; i < hexStringLength; i++) {
        if (!valid_hex_char(hexString[i])) {
            break;
        }
        number_hex_chars++;
    }

    if (number_hex_chars % 2 != 0) {
        return nullptr;
    }

    uint8_t *buff = (uint8_t *)malloc(number_hex_chars / 2);
    for (size_t i = 0; i < number_hex_chars / 2; i++) {
        uint8_t v = (char_to_uint8(hexString[i * 2]) << 4u) +
                    char_to_uint8(hexString[i * 2 + 1]);
        buff[i] = v;
    }
    return buff;
}

bool Interrupt_RemoteCall_deserialize_response(FunCallResponse *response,
                                               uint8_t *hex_encoded_response,
                                               const size_t size_response) {
    // format: error type msg
    // interrupt nr (1byte) | message_type (1 byte) | error_code (1 byte) |
    // newline

    // format: success type msg
    // format header: interrupt nr (1byte) | message_type (1 byte) | success
    // (1 byte)

    // format body 1: has_value (1 byte) | StackValue (optional) OR format
    // body 2: has_excp_msg (1 byte) | excp_msg (optional) end: newline

    // Only two possible types of responses
    uint8_t *encoded_response = hex_to_uint8_t((char *)hex_encoded_response);
    if (encoded_response[0] != interruptFunCall ||
        (encoded_response[1] != INTERRUPT_RESPONSE_TYPE_ERROR &&
         encoded_response[1] != INTERRUPT_RESPONSE_TYPE_SUCCESS)) {
        return false;
    }

    response->type = encoded_response[1];
    if (response->type == INTERRUPT_RESPONSE_TYPE_ERROR) {
        response->error_code = encoded_response[2];
        return true;
    }

    CallResult *result = new CallResult;
    result->success = encoded_response[2];
    result->value = nullptr;
    result->exception_msg = nullptr;
    bool has_val_or_excpmsg = encoded_response[3];
    if (has_val_or_excpmsg) {
        if (result->success) {
            const ValueSerializationConfig config{.includeIndex = false,
                                                  .includeType = true};
            result->value = new StackValue;
            deserializeStackValue(result->value, config, encoded_response + 4);
        } else {
            size_t str_size = size_response - 5;
            char *excp_msg = (char *)malloc(str_size);
            memcpy(excp_msg, encoded_response + 4, str_size);
            result->exception_msg = excp_msg;
        }
    }
    response->type = INTERRUPT_RESPONSE_TYPE_SUCCESS;
    response->result = result;
    return true;
}

bool sendRemoteCallRequest(Channel &channel, FunCallRequest &request,
                           uint8_t &error_code) {
    size_t request_size{};
    char *request_encoded =
        Interrupt_RemoteCall_serialize_request(request, request_size);
    if (request_encoded == nullptr) {
        error_code = REMOTE_CALL_ERROR_CODE_OUT_OF_MEMORY;
        return false;
    }

    int written = channel.write("%s\n", request_encoded);
    if (written == -1 || written != (request_size + 1)) {
        error_code = REMOTE_CALL_ERROR_CODE_WRITE_TO_CLIENT;
        return false;
    }
    return true;
}
bool waitForReply(Channel &channel, uint8_t &error_code, std::string &dest) {
    ChannelReader reader{channel};
    int number_bytes_read = reader.readLine(dest);
    if (number_bytes_read != -1) {
        // skip first line interrupt ack printed by
        // https://github.com/TOPLLab/WARDuino/blob/63a62b69d73936060314658a215d38e035c94f13/src/Debug/debugger.cpp#L173
        std::string ack = "Interrupt: ";
        if (dest.compare(0, ack.length(), ack) == 0) {
            number_bytes_read = reader.readLine(dest);
        }
    }

    if (number_bytes_read == -1) {
        error_code = REMOTE_CALL_ERROR_CODE_READ_FROM_CLIENT;
        return false;
    }
    return true;
}

void Interrupt_RemoteCall_call(const uint32_t func, StackValue *args,
                               const uint32_t args_size, Channel &channel,
                               FunCallResponse *response) {
    // prepare request
    FunCallRequest request;
    request.fun = func;
    request.number_args = args_size;
    request.args = args;
    uint8_t error_code;
    std::string encoded_response{};
    if (!sendRemoteCallRequest(channel, request, error_code) ||
        !waitForReply(channel, error_code, encoded_response)) {
        response->type = INTERRUPT_RESPONSE_TYPE_ERROR;
        response->error_code = error_code;
        return;
    }

    if (!Interrupt_RemoteCall_deserialize_response(
            response, (uint8_t *)encoded_response.c_str(),
            encoded_response.length())) {
        response->type = INTERRUPT_RESPONSE_TYPE_ERROR;
        response->error_code = REMOTE_CALL_ERROR_CODE_MALFORMED_RESPONSE;
    }
}