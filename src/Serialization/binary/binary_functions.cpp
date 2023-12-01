#include "binary_functions.h"

#include "../../Utils/macros.h"
#include "../../Utils/util.h"
#include "binary_multiple_objects.h"

void JSON_serialize_Functions(const Channel& channel, const Block* functions,
                              uint32_t begin, uint32_t end, const Module* mod) {
    JSON_preSerialization(channel);
    Module* m = (Module*)mod;
    channel.write("\"functions\":[");
    for (uint32_t i = begin; i < end; i++) {
        channel.write(R"({"fidx":"0x%x",)", functions[i].fidx);
        channel.write("\"from\":%" PRIu32 ",\"to\":%" PRIu32 "}%s",
                      toVirtualAddress(m->functions[i].start_ptr, m),
                      toVirtualAddress(m->functions[i].end_ptr, m),
                      (i < m->function_count - 1) ? "," : "],");
    }
    JSON_postSerialization(channel);
}