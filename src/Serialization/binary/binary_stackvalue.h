#pragma once

#include "../../Structs/struct_stackvalue.h"
#include "../../Utils/sockets.h"

void JSON_serialize_StackValue(const Channel& channel, const StackValue& value,
                               uint32_t value_idx);

void JSON_deserialize_StackValue(uint8_t* src, StackValue* dest_value);