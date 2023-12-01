#pragma once

#include "../../Structs/struct_table.h"
#include "../../Utils/sockets.h"

void BINARY_serialize_Table(const Channel& channel, const Table& table);

void BINARY_deserialize_Table(uint8_t* src, const Table& table);

