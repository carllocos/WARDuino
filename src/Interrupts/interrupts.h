#pragma once

#define INTERRUPT_RESPONSE_TYPE_SUCCESS 01
#define INTERRUPT_RESPONSE_TYPE_ERROR 02
#define INTERRUPT_RESPONSE_TYPE_SUBSCRIPTION 03

enum InterruptTypes {
    // Remote Debugging
    interruptRUN = 0x01,
    interruptHALT = 0x02,
    interruptPAUSE = 0x03,
    interruptSTEP = 0x04,
    interruptSTEPOver = 0x05,
    interruptBPAdd = 0x06,
    interruptBPRem = 0x07,
    interruptInspect = 0x09,
    interruptDUMP = 0x10,
    interruptDUMPLocals = 0x11,
    interruptDUMPFull = 0x12,
    interruptReset = 0x13,
    interruptUPDATEFun = 0x20,
    interruptUPDATELocal = 0x21,
    interruptUPDATEModule = 0x22,
    interruptUPDATEGlobal = 0x23,
    interruptUPDATEStackValue = 0x24,

    // Remote REPL
    interruptINVOKE = 0x40,
    interruptFunCall = 0x41,

    // Instrumentation
    interruptAroundFunction = 0x50,
    interruptMonitorAddr = 0x51,

    // Pull Debugging
    interruptSnapshot = 0x60,
    interruptLoadSnapshot = 0x62,
    interruptMonitorProxies = 0x63,
    interruptProxyCall = 0x64,
    interruptProxify = 0x65,  // wifi SSID \0 wifi PASS \0

    // Push Debugging
    interruptDUMPAllEvents = 0x70,
    interruptDUMPEvents = 0x71,
    interruptPOPEvent = 0x72,
    interruptPUSHEvent = 0x73,
    interruptDUMPCallbackmapping = 0x74,
    interruptRecvCallbackmapping = 0x75
};