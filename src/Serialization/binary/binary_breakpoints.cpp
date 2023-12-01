#include "binary_breakpoints.h"

#include "../../Utils/macros.h"
#include "../../Utils/util.h"

void BINARY_serialize_Breakpoints(const Channel& channel,
                                  const std::set<uint8_t*>& bps, Module* mod) {
    size_t encoding_size = 1 + size_for_LEB32(bps.size());
    for (auto bp : bps) {
        encoding_size += size_for_LEB32(toVirtualAddress(bp, mod));
    }
    uint8_t* encoding = (uint8_t*)malloc(encoding_size);
    encoding[0] = breakpointsState;
    size_t offset = 1;
    for (auto bp : bps) {
        offset += writeLEB32(toVirtualAddress(bp, mod), encoding + offset);
    }
}

BreakpointDeserialized BINARY_deserialize_Breakpoints(uint8_t* src) {
    FATAL("Breakpoints json deserialization not implemented\n");
}
