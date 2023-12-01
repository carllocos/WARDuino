#pragma once
#include <set>

#include "../../Structs/struct_module.h"
#include "../../Utils/sockets.h"
#include "../deserialized_structs.h"

void BINARY_serialize_Breakpoints(const Channel& channel,
                                  const std::set<uint8_t*>& bps,
                                  const Module* mod);

BreakpointDeserialized BINARY_deserialize_Breakpoints(uint8_t* src);
