#include "interrupt_monitor_addr.h"

#include "../Utils/util.h"
#include "interrupt_response.h"

/*
 * Declaration private functions
 */
bool registerMonitorAddrHook(InstrumentationManager &manager, Module &m,
                             const MonitorAddrRequest &request,
                             uint8_t &error_code);

/*
 * Public functions
 */

void Interrupt_MonitorAddr_handle_request(const Channel &channel,
                                          Module &module,
                                          InstrumentationManager &manager,
                                          uint8_t *encoded_request) {
    Hook hook{};
    MonitorAddrRequest request;
    request.hook = &hook;

    MonitorAddrResponse response;
    uint8_t error{};

    if (Interrupt_MonitorAddr_deserialize_request(request, encoded_request,
                                                  error) &&
        registerMonitorAddrHook(manager, module, request, error)) {
        response.type = INTERRUPT_RESPONSE_TYPE_SUCCESS;
    } else {
        response.type = INTERRUPT_RESPONSE_TYPE_ERROR;
        response.error_code = error;
    }

    Interrupt_MonitorAddr_send_response(channel, response);
}

bool Interrupt_MonitorAddr_deserialize_request(MonitorAddrRequest &dest,
                                               uint8_t *encoded_request,
                                               uint8_t &error_code) {
    // format: InterruptNr (1 byte)| addr (LEB32) | InstrumentMoment (1 byte) |
    // hook

    uint8_t *data = encoded_request;
    if (*data++ != interruptMonitorAddr) {
        error_code = MONITOR_ADDR_ERROR_CODE_REQUEST_HAS_WRONG_INTERRUPT_NR;
        return false;
    }
    dest.addr = read_LEB_32(&data);
    dest.moment = (InstrumentMoment)*data++;
    switch (dest.moment) {
        case InstrumentBefore:
        case InstrumentAfter:
            break;
        default:
            error_code = MONITOR_ADDR_ERROR_CODE_REQUEST_HAS_UN_EXISTING_MOMENT;
            return false;
    }

    return Hooks_deserialize_hook(*dest.hook, &data, error_code);
}

ssize_t Interrupt_MonitorAddr_serialize_response(
    const MonitorAddrResponse &response, char *dest) {
    return Interrupt_serialize_JSON_response(
        interruptMonitorAddr, response.type, response.error_code, dest);
}

void Interrupt_MonitorAddr_send_response(const Channel &channel,
                                         const MonitorAddrResponse &response) {
    char buffer[100]{};
    if (Interrupt_MonitorAddr_serialize_response(response, buffer) > 0) {
        channel.write(buffer);
    }
}

/*
 * Private functions
 */

bool registerMonitorAddrHook(InstrumentationManager &manager, Module &m,
                             const MonitorAddrRequest &request,
                             uint8_t &error_code) {
    if (!isToPhysicalAddrPossible(request.addr, &m)) {
        error_code = MONITOR_ADDR_ERROR_CODE_REQUEST_HAS_UNEXISTING_ADDR;
        return false;
    }
    if (!manager.addHookOnOnWasmAddress(m, request.addr, *request.hook,
                                        request.moment)) {
        error_code = MONITOR_ADDR_ERROR_CODE_COULD_NOT_ADD_HOOK;
        return false;
    }
    return true;
}

void Interrupt_MonitorAddr_send_JSON_subscribe_message(
    const Channel &output, InstrumentMoment moment, uint32_t addr,
    std::function<void()> hookOutput) {
    auto subscriptionMsgBody = [&output, moment, hookOutput, addr]() {
        output.write(R"({"moment":"%02X","addr":"%02X","val":)", moment, addr);
        hookOutput();
        output.write("}");
    };
    Interrupt_send_JSON_subscribe_message(output, interruptMonitorAddr,
                                          subscriptionMsgBody);
}
