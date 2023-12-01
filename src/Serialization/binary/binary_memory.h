#pragma once

#include "../../Structs/struct_memory.h"
#include "../../Utils/sockets.h"
#include "../../WARDuino/WARDuino_constants.h"

void JSON_serialize_Memory(const Channel& channel, const Memory& memory);

void JSON_deserialize_Memory(uint8_t* src, Memory* memory);
