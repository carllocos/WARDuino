#include "json_globals.h"

#include "../../Utils/macros.h"
#include "json_multiple_objects.h"
#include "json_stackvalue.h"

void JSON_serialize_Globals(const Channel& channel, const StackValue* globals,
                            uint32_t begin, uint32_t end) {
    JSON_preSerialization(channel);
    channel.write("\"globals\":[");
    for (uint32_t j = begin; j < end; j++) {
        auto v = globals[j];
        JSON_serialize_StackValue(channel, v, j);
        channel.write("%s", j + 1 < end ? "," : "");
    }
    channel.write("]");
    JSON_postSerialization(channel);
}

GlobalsDeserialized JSON_deserialize_Globals(uint8_t* src) {
    FATAL("Globals json deserialization not implemented");
}
