#include "interrupt_monitor_addr.h"

#include "../Utils/util.h"
#include "interrupt_response.h"

/*
 * Declaration private functions
 */
bool registerMonitorAddrAction(InstrumentationManager &manager, Module &m,
                               const MonitorAddrRequest &request,
                               uint8_t &error_code);

/*
 * Public functions
 */

void Interrupt_MonitorAddr_handle_request(const Channel &channel,
                                          const Module &module,
                                          InstrumentationManager &manager,
                                          Module *m, uint8_t *encoded_request) {
    Action action{};
    MonitorAddrRequest request;
    request.action = &action;

    MonitorAddrResponse response;
    uint8_t error{};

    if (Interrupt_MonitorAddr_deserialize_request(module, request,
                                                  encoded_request, error) &&
        registerMonitorAddrAction(manager, *m, request, error)) {
        response.type = INTERRUPT_RESPONSE_TYPE_SUCCESS;
    } else {
        response.type = INTERRUPT_RESPONSE_TYPE_ERROR;
        response.error_code = error;
    }

    Interrupt_MonitorAddr_send_response(channel, response);
}

bool Interrupt_MonitorAddr_deserialize_request(const Module &module,
                                               MonitorAddrRequest &dest,
                                               uint8_t *encoded_request,
                                               uint8_t &error_code) {
    // format: InterruptNr (1 byte)| addr (LEB32) | InstrumentMoment (1 byte) |
    // action

    uint8_t *data = encoded_request;
    if (*data++ != interruptMonitorAddr) {
        error_code = MONITOR_ADDR_ERROR_CODE_REQUEST_HAS_WRONG_INTERRUPT_NR;
        return false;
    }
    dest.addr = read_LEB_32(&data);
    if (!isToPhysicalAddrPossible(dest.addr, (Module *)&module)) {
        error_code = MONITOR_ADDR_ERROR_CODE_REQUEST_HAS_UNEXISTING_ADDR;
        return false;
    }
    dest.moment = (InstrumentMoment)*data++;
    switch (dest.moment) {
        case InstrumentBefore:
        case InstrumentAfter:
            break;
        default:
            error_code = MONITOR_ADDR_ERROR_CODE_REQUEST_HAS_UN_EXISTING_MOMENT;
            return false;
    }

    return Actions_deserialize_action(*dest.action, &data, error_code);
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

bool registerMonitorAddrAction(InstrumentationManager &manager, Module &m,
                               const MonitorAddrRequest &request,
                               uint8_t &error_code) {}
