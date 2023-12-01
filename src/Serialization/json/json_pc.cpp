#include "json_pc.h"

#include <inttypes.h>

#include "../../Utils/macros.h"
#include "../../Utils/util.h"
#include "json_multiple_objects.h"

void JSON_serialize_PC(const Channel& channel, const uint8_t* pc,
                       const Module* mod) {
    Module* m = (Module*)mod;
    uint8_t* pc_t = (uint8_t*)pc;
    JSON_preSerialization(channel);
    channel.write("\"pc\":%" PRIu32 "", toVirtualAddress(pc_t, m));
    JSON_postSerialization(channel);
}

uint32_t JSON_deserialize_PC(uint8_t* src) {
    FATAL("PC json deserialization not implemented\n");
}
