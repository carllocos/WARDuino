#pragma once
#include "../../Structs/struct_stackvalue.h"
#include "../../Utils/sockets.h"
#include "../deserialized_structs.h"

void JSON_serialize_Stack(const Channel& channel, const StackValue* stack,
                          int begin, int end);

StackDeserialized JSON_deserialize_Stack(uint8_t* src);