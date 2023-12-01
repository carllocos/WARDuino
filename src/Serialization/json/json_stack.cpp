#include "json_stack.h"

#include "../../Utils/macros.h"
#include "json_multiple_objects.h"
#include "json_stackvalue.h"

void JSON_serialize_Stack(const Channel& channel, const StackValue* stack,
                          int begin, int end) {
    JSON_preSerialization(channel);
    channel.write("\"stack\":[");
    for (int j = begin; j <= end; j++) {
        JSON_serialize_StackValue(channel, stack[j], j);
        channel.write("%s", (j + 1) <= end ? "," : "");
    }
    channel.write("]");
    JSON_postSerialization(channel);
}
StackDeserialized JSON_deserialize_Stack(uint8_t* src) {
    FATAL("Stack json deserialization not implemented\n");
}
