#include "json_breakpoints.h"

#include <inttypes.h>

#include "../../Utils/macros.h"
#include "../../Utils/util.h"
#include "json_multiple_objects.h"

void JSON_serialize_Breakpoints(const Channel& channel,
                                const std::set<uint8_t*>& bps,
                                const Module* mod) {
    JSON_preSerialization(channel);
    channel.write("\"breakpoints\":[");
    size_t i = 0;
    Module* m = (Module*)mod;
    for (auto bp : bps) {
        channel.write("%" PRIu32 "%s", toVirtualAddress(bp, m),
                      (++i < bps.size()) ? "," : "");
    }
    channel.write("]");
    JSON_postSerialization(channel);
}

BreakpointDeserialized JSON_deserialize_Breakpoints(uint8_t* src) {
    FATAL("Breakpoints json deserialization not implemented\n");
}
