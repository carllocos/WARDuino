#pragma once
#include <cstdint>

typedef struct Table {
    uint8_t elem_type = 0;  // type of entries (only ANYFUNC in MVP)
    uint32_t initial = 0;   // initial table size
    uint32_t maximum = 0;   // maximum table size
    uint32_t size = 0;      // current table size
    uint32_t *entries = nullptr;
} Table;

