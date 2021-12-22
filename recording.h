#ifndef RECORDING_H
#define RECORDING_H

#include <vector>
#include <cstdint>
#include "structs.h"

struct record {
    uint32_t id;
    uint32_t fidx;
    StackValue value;
};

#endif  // RECORDING_H
