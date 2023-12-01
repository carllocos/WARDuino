#pragma once
#include "../../Structs/struct_module.h"
#include "../../Utils/sockets.h"

void JSON_serialize_PC(const Channel& channel, const uint8_t* pc,
                       const Module* mod);

uint32_t JSON_deserialize_PC(uint8_t* src);