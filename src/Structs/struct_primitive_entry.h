#pragma once
#include "struct_module.h"
#include "struct_type.h"

typedef bool (*Primitive)(Module *);

typedef struct PrimitiveEntry {
    const char *name;
    Primitive f;
    Type t;
} PrimitiveEntry;
