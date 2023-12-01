#pragma once

#include "../../Utils/sockets.h"
#include "../../WARDuino/CallbackHandler.h"
#include "../deserialized_structs.h"

void JSON_serialize_Event(const Channel& channel, const Event& event);

void JSON_deserialize_Event(uint8_t* src, Event& src_event);

void JSON_serialize_Events(const Channel& channel,
                           std::deque<Event>::const_iterator start,
                           std::deque<Event>::const_iterator end);

EventsDeserialized JSON_deserialize_Events(uint8_t* src);