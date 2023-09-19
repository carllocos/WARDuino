#pragma once
#include <cstdint>

typedef struct Memory {
    uint32_t initial = 0;      // initial size (64K pages)
    uint32_t maximum = 0;      // maximum size (64K pages)
    uint32_t pages = 0;        // current size (64K pages)
    uint8_t *bytes = nullptr;  // memory area
} Memory;
