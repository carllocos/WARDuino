#pragma once

#include "../../Utils/sockets.h"
#include "../../WARDuino/CallbackHandler.h"
#include "vector"

void BINARY_serialize_CallbackMapping(const Channel& channel, std::string topic,
                                      std::vector<Callback>* cbs);
void BINARY_deserialize_CallbackMapping(uint8_t* src, std::string* dest_id,
                                        Callback* dest_cb);

void BINARY_serialize_CallbackMappings(const Channel& channel,
                                       CallbackHandler::callbackIterator start,
                                       CallbackHandler::callbackIterator end);

std::unordered_map<std::string, std::vector<Callback>>*
BINARY_deserialize_CallbackMappings(uint8_t* src);