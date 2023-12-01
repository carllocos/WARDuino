#include "binary_memory.h"

#include <inttypes.h>

#include "../../Utils/macros.h"
#include "binary_multiple_objects.h"

void JSON_serialize_Memory(const Channel& channel, const Memory& memory) {
    JSON_preSerialization(channel);
    uint32_t total_elems = memory.pages * (uint32_t)PAGE_SIZE;
    channel.write(R"("memory":{"pages":%d,"max":%d,"init":%d,"bytes":[)",
                  memory.pages, memory.maximum, memory.initial);
    for (uint32_t j = 0; j < total_elems; j++) {
        channel.write("%" PRIu8 "%s", memory.bytes[j],
                      (j + 1) == total_elems ? "" : ",");
    }
    channel.write("]}");  // closing memory
    JSON_postSerialization(channel);
}

void JSON_deserialize_Memory(uint8_t* src, Memory* memory) {
    FATAL("Memory json deserialization not implemented\n");
}
