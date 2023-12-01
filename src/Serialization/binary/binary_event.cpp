#include "binary_event.h"

#include "../../Utils/macros.h"
#include "binary_multiple_objects.h"

void JSON_serialize_Event(const Channel& channel, const Event& event) {
    channel.write(R"({"topic": "%s", "payload": "%s"})", event.topic.c_str(),
                  event.payload.c_str());
}
void JSON_deserialize_Event(uint8_t* src, Event& src_event) {
    FATAL("Event json deserialization not implemented\n");
}

void JSON_serialize_Events(const Channel& channel,
                           std::deque<Event>::const_iterator start,
                           std::deque<Event>::const_iterator end) {
    bool previous = CallbackHandler::resolving_event;
    CallbackHandler::resolving_event = true;

    if (std::distance(end, CallbackHandler::event_end()) > 0) {
        end = CallbackHandler::event_end();
    }

    JSON_preSerialization(channel);
    channel.write(R"("events":[)");
    for (auto event = start; event != end; event++) {
        JSON_serialize_Event(channel, *event);
        if (event + 1 != end) {
            channel.write(", ");
        }
    }
    channel.write("]");
    CallbackHandler::resolving_event = previous;
    JSON_postSerialization(channel);
}
EventsDeserialized JSON_deserialize_Events(uint8_t* src) {
    FATAL("Events json deserialization not implemented\n");
}
