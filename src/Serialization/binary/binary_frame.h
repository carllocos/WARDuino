#pragma once
#include "../../Structs/struct_frame.h"
#include "../../Structs/struct_module.h"
#include "../../Utils/sockets.h"

void BINARY_serialize_Frame(const Channel& channel, const Frame* frame,
                            int frame_index, const Module* mod);

void BINARY_deserialize_Frame(uint8_t* src, Frame* dest_frame);