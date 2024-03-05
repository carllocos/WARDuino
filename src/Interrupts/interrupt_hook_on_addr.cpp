#include "interrupt_hook_on_addr.h"

#include "../Utils/util.h"
#include "interrupt_response.h"

/*
 * Declaration private functions
 */
bool registerHookOnAddr(InstrumentationManager &manager, Module &m,
                        const HookOnAddrRequest &request, uint8_t &error_code);

/*
 * Public functions
 */

void Interrupt_HookOnAddr_handle_request(const Channel &channel, Module &module,
                                         InstrumentationManager &manager,
                                         uint8_t *encoded_request) {
    Hook hook{};
    HookOnAddrRequest request;
    request.hook = &hook;

    HookOnAddrResponse response;
    uint8_t error{};

    if (Interrupt_HookOnAddr_deserialize_request(request, encoded_request,
                                                 error) &&
        registerHookOnAddr(manager, module, request, error)) {
        response.type = INTERRUPT_RESPONSE_TYPE_SUCCESS;
    } else {
        response.type = INTERRUPT_RESPONSE_TYPE_ERROR;
        response.error_code = error;
    }

    Interrupt_HookOnAddr_send_response(channel, response);
}

bool Interrupt_HookOnAddr_deserialize_request(HookOnAddrRequest &dest,
                                              uint8_t *encoded_request,
                                              uint8_t &error_code) {
    // format: InterruptNr (1 byte)| addr (LEB32) | HookMoment (1 byte)
    // | add or remove (1 byte) |hook

    uint8_t *data = encoded_request;
    if (*data++ != interruptHookOnAddress) {
        error_code = HOOK_ON_ADDR_ERROR_CODE_REQUEST_HAS_WRONG_INTERRUPT_NR;
        return false;
    }
    dest.addr = read_LEB_32(&data);
    dest.moment = (HookMoment)*data++;
    switch (dest.moment) {
        case HookBefore:
        case HookAfter:
            break;
        default:
            error_code = HOOK_ON_ADDR_ERROR_CODE_REQUEST_HAS_UN_EXISTING_MOMENT;
            return false;
    }

    dest.add = *data++;
    if (dest.add) {
        return Hooks_deserialize_hook(*dest.hook, &data, error_code);
    }
    // removing hooks so nothing to deserialize
    dest.hook = nullptr;
    return true;
}

ssize_t Interrupt_HookOnAddr_serialize_response(
    const HookOnAddrResponse &response, char *dest) {
    return Interrupt_serialize_JSON_response(
        interruptHookOnAddress, response.type, response.error_code, dest);
}

void Interrupt_HookOnAddr_send_response(const Channel &channel,
                                        const HookOnAddrResponse &response) {
    char buffer[100]{};
    if (Interrupt_HookOnAddr_serialize_response(response, buffer) > 0) {
        channel.write(buffer);
    }
}

/*
 * Private functions
 */

bool registerHookOnAddr(InstrumentationManager &manager, Module &m,
                        const HookOnAddrRequest &request, uint8_t &error_code) {
    if (!isToPhysicalAddrPossible(request.addr, &m)) {
        error_code = HOOK_ON_ADDR_ERROR_CODE_REQUEST_HAS_UNEXISTING_ADDR;
        return false;
    }
    if (request.add) {
        if (!manager.addHookOnWasmAddress(m, request.addr, *request.hook,
                                          request.moment)) {
            error_code = HOOK_ON_ADDR_ERROR_CODE_COULD_NOT_ADD_HOOK;
            return false;
        }
    } else {
        if (!manager.removeHooksOnWasmAddress(m, request.addr,
                                              request.moment)) {
            error_code = HOOK_ON_ADDR_ERROR_CODE_COULD_NOT_REMOVE_HOOK;
            return false;
        }
    }
    return true;
}

void Interrupt_HookOnAddr_send_JSON_subscribe_message(
    const Channel &output, HookMoment moment, uint32_t addr,
    std::function<void()> hookOutput) {
    auto subscriptionMsgBody = [&output, moment, hookOutput, addr]() {
        output.write(R"({"moment":"%02X","addr":"%02X","val":)", moment, addr);
        hookOutput();
        output.write("}");
    };
    Interrupt_send_JSON_subscribe_message(output, interruptHookOnAddress,
                                          subscriptionMsgBody);
}
