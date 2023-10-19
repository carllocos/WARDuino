#include "interrupt_response.h"

#include <stdio.h>

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
                "unknown %02\n",
                response_type);
            return -1;
    }
    offset += sprintf(dest + offset, "}\n");
    return offset;
}