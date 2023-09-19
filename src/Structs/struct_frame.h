#pragma once
#include <cstdint>

#include "struct_block.h"

typedef struct Frame {
    Block *block;
    // Saved state
    int sp;
    int fp;
    uint8_t *ra_ptr;
} Frame;
