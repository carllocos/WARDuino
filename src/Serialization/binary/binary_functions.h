#pragma once

#include <inttypes.h>

#include "../../Structs/struct_block.h"
#include "../../Structs/struct_module.h"
#include "../../Utils/sockets.h"
#include "../deserialized_structs.h"

/*
 * Functions do not need to be deserialized as they cannot be loaded into the VM
 */
void JSON_serialize_Functions(const Channel& channel, const Block* functions,
                              uint32_t begin, uint32_t end, const Module* mod);