#include "binary_callback_mapping.h"

#include <inttypes.h>

#include "../../Utils/macros.h"
#include "binary_multiple_objects.h"

void JSON_serialize_CallbackMapping(const Channel& channel, std::string topic,
                                    std::vector<Callback>* cbs) {
    channel.write(R"({"%s": [)", topic.c_str());
    auto callback = std::begin(*cbs);
    while (callback != std::end(*cbs)) {
        channel.write("%" PRIu32 "%s", callback->table_index,
                      (++callback != std::end(*cbs)) ? ", " : "");
    }
    channel.write(R"(]})");
}

void BINARY_deserialize_CallbackMapping(uint8_t* src, std::string* dest_id,
                                        Callback* dest_cb) {
    FATAL("CallbackMapping json deserialization not implemented\n");
}

void BINARY_serialize_CallbackMappings(const Channel& channel,
                                       CallbackHandler::callbackIterator start,
                                       CallbackHandler::callbackIterator end) {
    BINARY_preSerialization(channel);
    channel.write(R"("callbacks": [)");
    while (start != end) {
        BINARY_serialize_CallbackMapping(channel, start->first, start->second);
        channel.write("%s", (++start != end) ? ", " : "");
    }
    channel.write("]");
    BINARY_postSerialization(channel);
}

std::unordered_map<std::string, std::vector<Callback>>*
BINARY_deserialize_CallbackMappings(uint8_t* src) {
    FATAL("CallbackMappings json deserialization not implemented\n");
}