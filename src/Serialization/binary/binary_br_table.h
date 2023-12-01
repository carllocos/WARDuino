#pragma once
#include <cstdint>

#include "../../Utils/sockets.h"

void BINARY_serialize_BranchingTable(const Channel& channel,
                                   const uint32_t* br_table);

void BINARY_deserialize_BranchingTable(uint8_t* src, uint32_t* br_table);