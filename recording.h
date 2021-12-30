#ifndef RECORDING_H
#define RECORDING_H

#include <vector>
#include <cstdint>
#include "structs.h"

typedef struct {
    uint32_t id;
    uint32_t fidx;
    StackValue value;
} Record;

#endif  // RECORDING_H
