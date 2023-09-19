#pragma once

#include <cstdint>
#include <map>

#include "struct_block.h"
#include "struct_frame.h"
#include "struct_memory.h"
#include "struct_options.h"
#include "struct_stackvalue.h"
#include "struct_table.h"
#include "struct_type.h"

class WARDuino;  // predeclare for it work in the module decl

typedef struct Module {
    WARDuino *warduino = nullptr;
    char *path = nullptr;  // file path of the wasm module
    Options options;       // Config options

    uint32_t byte_count = 0;   // number of bytes in the module
    uint8_t *bytes = nullptr;  // module content/bytes

    uint32_t type_count = 0;  // number of function types
    Type *types = nullptr;    // function types

    uint32_t import_count = 0;    // number of leading imports in functions
    uint32_t function_count = 0;  // number of function (including imports)
    Block *functions = nullptr;   // imported and locally defined functions
    std::map<uint8_t *, Block *>
        block_lookup;  // map of module byte position to Blocks
    // same length as byte_count
    uint32_t start_function = -1;  // function to run on module load
    Table table;
    Memory memory;
    uint32_t global_count = 0;      // number of globals
    StackValue *globals = nullptr;  // globals
    // Runtime state
    uint8_t *pc_ptr = nullptr;     // program counter
    int sp = -1;                   // operand stack pointer
    int fp = -1;                   // current frame pointer into stack
    StackValue *stack = nullptr;   // main operand stack
    int csp = -1;                  // callstack pointer
    Frame *callstack = nullptr;    // callstack
    uint32_t *br_table = nullptr;  // br_table branch indexes

    char *exception = nullptr;  // exception is set when the program fails
} Module;
