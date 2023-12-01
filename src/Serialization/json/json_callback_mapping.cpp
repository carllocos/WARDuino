#include "json_callback_mapping.h"

#include <inttypes.h>

#include "../../Utils/macros.h"
#include "json_multiple_objects.h"

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

void JSON_deserialize_CallbackMapping(uint8_t* src, std::string* dest_id,
                                      Callback* dest_cb) {
    FATAL("CallbackMapping json deserialization not implemented\n");
}

void JSON_serialize_CallbackMappings(const Channel& channel,
                                     CallbackHandler::callbackIterator start,
                                     CallbackHandler::callbackIterator end) {
    JSON_preSerialization(channel);
    channel.write(R"("callbacks": [)");
    while (start != end) {
        JSON_serialize_CallbackMapping(channel, start->first, start->second);
        channel.write("%s", (++start != end) ? ", " : "");
    }
    channel.write("]");
    JSON_postSerialization(channel);
}

std::unordered_map<std::string, std::vector<Callback>>*
JSON_deserialize_CallbackMappings(uint8_t* src) {
    FATAL("CallbackMappings json deserialization not implemented\n");
}

// TODO remove once plugin and VM no longer uses vers2
void JSON_serialize_CallbackMappingVers2(const Channel& channel,
                                         std::string topic,
                                         std::vector<Callback>* cbs) {
    channel.write(R"({"callbackid": "%s", "tableIndexes": [)", topic.c_str());
    auto callback = std::begin(*cbs);
    while (callback != std::end(*cbs)) {
        channel.write("%" PRIu32 "%s", callback->table_index,
                      (++callback != std::end(*cbs)) ? ", " : "");
    }
    channel.write(R"(]})");
}

// TODO remove once plugin and VM no longer uses vers2
void JSON_serialize_CallbackMappingsVers2(
    const Channel& channel, CallbackHandler::callbackIterator start,
    CallbackHandler::callbackIterator end) {
    JSON_preSerialization(channel);
    channel.write(R"("callbacks": [)");
    while (start != end) {
        JSON_serialize_CallbackMappingVers2(channel, start->first,
                                            start->second);
        channel.write("%s", (++start != end) ? ", " : "");
    }
    channel.write("]");
    JSON_postSerialization(channel);
}

// TODO remove once plugin and VM no longer uses vers2
std::unordered_map<std::string, std::vector<Callback>>*
JSON_deserialize_CallbackMappingsVers2(uint8_t* src) {
    FATAL("CallbackMappingsVers2 json deserialization not implemented\n");
}
