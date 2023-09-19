#pragma once

#include <cstdint>

#include "struct_type.h"

// A block or function
typedef struct Block {
    uint8_t block_type;  // 0x00: function, 0x01: init_exp, 0x02: block,
    // 0x03: loop, 0x04: if, 0xfe: proxy guard,
    // 0xff: cbk guard
    uint32_t fidx;              // function only (index)
    Type *type;                 // params/results type
    uint32_t local_count;       // function only
    uint8_t *local_value_type;  // types of locals (function only)
    uint8_t *start_ptr;
    uint8_t *end_ptr;
    uint8_t *else_ptr;    // if block only
    uint8_t *br_ptr;      // blocks only
    char *export_name;    // function only (exported)
    char *import_module;  // function only (imported)
    char *import_field;   // function only (imported)
    void (*func_ptr)();   // function only (imported)
} Block;