#include "json_callstack.h"

#include "../../Utils/macros.h"
#include "json_frame.h"
#include "json_multiple_objects.h"

void JSON_serialize_Callstack(const Channel& channel, const Frame* callstack,
                              int begin, int end, const Module* m) {
    JSON_preSerialization(channel);
    channel.write("\"callstack\":[");
    for (int j = begin; j <= end; j++) {
        JSON_serialize_Frame(channel, &callstack[j], j, m);
        channel.write("%s", (j < end) ? "," : "");
    }
    channel.write("]");
    JSON_postSerialization(channel);
}
FrameDeserialized JSON_deserialize_Callstack(uint8_t* src) {
    FATAL("Callstack json deserialization not implemented\n");
}
