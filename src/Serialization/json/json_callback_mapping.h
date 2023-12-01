#pragma once

#include "../../Utils/sockets.h"
#include "../../WARDuino/CallbackHandler.h"
#include "vector"

void JSON_serialize_CallbackMapping(const Channel& channel, std::string topic,
                                    std::vector<Callback>* cbs);
void JSON_deserialize_CallbackMapping(uint8_t* src, std::string* dest_id,
                                      Callback* dest_cb);

void JSON_serialize_CallbackMappings(const Channel& channel,
                                     CallbackHandler::callbackIterator start,
                                     CallbackHandler::callbackIterator end);

std::unordered_map<std::string, std::vector<Callback>>*
JSON_deserialize_CallbackMappings(uint8_t* src);

// TODO remove once plugin and VM no longer uses vers2
void JSON_serialize_CallbackMappingVers2(const Channel& channel,
                                         std::string topic,
                                         std::vector<Callback>* cbs);
// TODO remove once plugin and VM no longer uses vers2
void JSON_serialize_CallbackMappingsVers2(
    const Channel& channel, CallbackHandler::callbackIterator start,
    CallbackHandler::callbackIterator end);

// TODO remove once plugin and VM no longer uses vers2
std::unordered_map<std::string, std::vector<Callback>>*
JSON_deserialize_CallbackMappingsVers2(uint8_t* src);