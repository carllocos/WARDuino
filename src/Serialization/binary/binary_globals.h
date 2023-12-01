#pragma once

#include "../../Structs/struct_stackvalue.h"
#include "../../Utils/sockets.h"
#include "../deserialized_structs.h"

void JSON_serialize_Globals(const Channel& channel, const StackValue* globals,
                            uint32_t begin, uint32_t end);

GlobalsDeserialized JSON_deserialize_Globals(uint8_t* src);