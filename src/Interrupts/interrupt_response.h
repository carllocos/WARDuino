#pragma once
#include <unistd.h>

#include <cstdint>

#include "./interrupts.h"

#define INTERRUPT_RESPONSE_TYPE_SUCCESS 01
#define INTERRUPT_RESPONSE_TYPE_ERROR 02
#define INTERRUPT_RESPONSE_TYPE_SUBSCRIPTION 03

/**
 * Function that serializes a generic interrupt response as a json string into
 * `dest`. Function ignores error_code if
 *
 * @param interrupt_nr the interrupt number for which the response is being
 * serialized
 * @param response_type type of response (either
 * INTERRUPT_RESPONSE_TYPE_SUCCESS, INTERRUPT_RESPONSE_TYPE_ERROR, or
 * INTERRUPT_RESPONSE_TYPE_SUBSCRIPTION)
 * @param error_code the error number that occurred. `error_code` is isgnored
 * when `response_type != INTERRUPT_RESPONSE_TYPE_ERROR`.
 * @param dest where to write the serialized response
 * @return returns nr of bytes written in `dest`. Returns -1 in when an error
 * occurred during serialization
 */
ssize_t Interrupt_serialize_JSON_response(const InterruptTypes interrupt_nr,
                                          const uint8_t response_type,
                                          const uint8_t error_code, char *dest);