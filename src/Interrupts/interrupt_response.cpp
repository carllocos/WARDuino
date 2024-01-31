#include "interrupt_response.h"

#include <stdio.h>

#include "../Utils/util.h"

ssize_t Interrupt_serialize_JSON_response(const InterruptTypes interrupt_nr,
                                          const uint8_t response_type,
                                          const uint8_t error_code,
                                          char *dest) {
    ssize_t offset = 0;
    offset += sprintf(dest, R"({"interrupt":"%02X","kind":"%02X")",
                      interrupt_nr, response_type);
    switch (response_type) {
        case INTERRUPT_RESPONSE_TYPE_ERROR:
            offset +=
                sprintf(dest + offset, R"(,"error_code":"%02X")", error_code);
        case INTERRUPT_RESPONSE_TYPE_SUCCESS:
            break;
        default:
            printf(
                "Interrupt_serialize_json_response: response type is "
                "unknown %02X\n",
                response_type);
            return -1;
    }
    offset += sprintf(dest + offset, "}\n");
    return offset;
}

void Interrupt_send_JSON_subscribe_message(
    const Channel &output, InterruptTypes interrupt_nr,
    std::function<void()> outputMessageBody) {
    output.write(R"({"interrupt":"%02X","kind":"%02X","sub":)", interrupt_nr,
                 INTERRUPT_RESPONSE_TYPE_SUBSCRIPTION);
    outputMessageBody();
    output.write("}\n");
}

ssize_t Interrupt_serialize_hexa_string_response(
    const InterruptTypes interrupt_nr, const uint8_t response_type,
    char *dest) {
    // format: interrupt_nr (1byte) | message_type (1byte)
    uint8_t buffer[2] = {};
    buffer[0] = interrupt_nr;
    buffer[1] = response_type;
    HexUInt8Encoding result{};
    result.encoding = dest;
    uint8_to_hex(buffer, 2, &result);
    return result.bytesWritten;
}

void getHumanReadableInterrupt(std::string &s, uint8_t interruptNr) {
    switch (interruptNr) {
        case interruptRUN:
            s = "interruptRun";
            break;
        case interruptHALT:
            s = "interruptHALT";
            break;
        case interruptPAUSE:
            s = "interruptPAUSE";
            break;
        case interruptSTEP:
            s = "interruptSTEP";
            break;
        case interruptSTEPOver:
            s = "interruptSTEPOver";
            break;
        case interruptBPAdd:
            s = "interruptBPAdd";
            break;
        case interruptBPRem:
            s = "interruptBPRem";
            break;
        case interruptInspect:
            s = "interruptInspect";
            break;
        case interruptDUMP:
            s = "interruptDUMP";
            break;
        case interruptDUMPLocals:
            s = "interruptDUMPLocals";
            break;
        case interruptDUMPFull:
            s = "interruptDUMPFull";
            break;
        case interruptReset:
            s = "interruptReset";
            break;
        case interruptUPDATEFun:
            s = "interruptUPDATEFun";
            break;
        case interruptUPDATELocal:
            s = "interruptUPDATELocal";
            break;
        case interruptUPDATEModule:
            s = "interruptUPDATEModule";
            break;
        case interruptUPDATEGlobal:
            s = "interruptUPDATEGlobal";
            break;
        case interruptUPDATEStackValue:
            s = "interruptUPDATEStackValue";
            break;
        case interruptINVOKE:
            s = "interruptINVOKE";
            break;
        case interruptFunCall:
            s = "interruptFunCall";
            break;
        case interruptAroundFunction:
            s = "interruptAroundFunction";
            break;
        case interruptHookOnAddress:
            s = "interruptHookOnAddress";
            break;
        case interruptHookOnEvent:
            s = "interruptHookOnEvent";
            break;
        case interruptSnapshot:
            s = "interruptSnapshot";
            break;
        case interruptLoadSnapshot:
            s = "interruptLoadSnapshot";
            break;
        case interruptMonitorProxies:
            s = "interruptMonitorProxies";
            break;
        case interruptProxyCall:
            s = "interruptProxyCall";
            break;
        case interruptProxify:
            s = "interruptProxify";
            break;
        case interruptDUMPAllEvents:
            s = "interruptDUMPAllEvents";
            break;
        case interruptDUMPEvents:
            s = "interruptDUMPEvents";
            break;
        case interruptPOPEvent:
            s = "interruptPOPEvent";
            break;
        case interruptPUSHEvent:
            s = "interruptPUSHEvent";
            break;
        case interruptDUMPCallbackmapping:
            s = "interruptDUMPCallbackmapping";
            break;
        case interruptRecvCallbackmapping:
            s = "interruptRecvCallbackmappin";
            break;
        default:
            s = "unknwon interrupt";
            break;
    }
}