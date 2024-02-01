#pragma once

#include <string>

#include "structs.h"

class Event {
   public:
    std::string topic;
    std::string payload;

    Event(std::string topic, std::string payload);

    std::string serialized() const;
};

class Callback {
   public:
    Module *module;  // reference to module
    std::string topic;
    uint32_t table_index{};

    explicit Callback(Module *m, std::string id, uint32_t tidx);
    Callback(const Callback &c);

    void resolve_event(const Event &e);
};
