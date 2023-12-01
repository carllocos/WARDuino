#pragma once
#include "../../Structs/struct_frame.h"
#include "../../Structs/struct_module.h"
#include "../../Utils/sockets.h"
#include "../serialization_strategy.h"

void BINARY_serialize_Callstack(const Channel& channel, const Frame* callstack,
                                int begin, int end, const Module* m);

FrameDeserialized BINARY_deserialize_Callstack(uint8_t* src);