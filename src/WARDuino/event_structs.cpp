#include "event_structs.h"

#include "../Interpreter/instructions.h"
#include "../Utils/macros.h"

// Event class

Event::Event(std::string topic, std::string payload) {
    this->topic = topic;
    this->payload = payload;
}

std::string Event::serialized() const {
    return R"({"topic": ")" + this->topic + R"(", "payload": ")" +
           this->payload + R"("})";
}

// Callback class

Callback::Callback(Module *m, std::string id, uint32_t tidx) {
    this->module = m;
    this->topic = std::move(id);
    this->table_index = tidx;
}

void Callback::resolve_event(const Event &e) {
    dbg_trace("Callback(%s, %i): resolving Event(%s, \"%s\")\n", topic.c_str(),
              table_index, e.topic.c_str(), e.payload);

    // Copy topic and payload to linear memory
    uint32_t start = 10000;  // TODO use reserved area in linear memory
    std::string topic = e.topic;
    std::string payload = e.payload;
    for (unsigned long i = 0; i < topic.length(); i++) {
        module->memory.bytes[start + i] = (uint32_t)e.topic[i];
    }
    start += topic.length();
    for (unsigned long i = 0; i < payload.length(); i++) {
        module->memory.bytes[start + i] = (uint32_t)e.payload[i];
    }

    // Push arguments (5 args)
    module->stack[++module->sp].value.uint32 = start - topic.length();
    module->stack[module->sp].value_type = I32;
    module->stack[++module->sp].value.uint32 = topic.length();
    module->stack[module->sp].value_type = I32;
    module->stack[++module->sp].value.uint32 = start;
    module->stack[module->sp].value_type = I32;
    module->stack[++module->sp].value.uint32 = payload.length();
    module->stack[module->sp].value_type = I32;
    module->stack[++module->sp].value.uint32 = payload.length();
    module->stack[module->sp].value_type = I32;

    // Setup function
    uint32_t fidx = module->table.entries[table_index];
    setup_call(module, fidx);

    // Validate argument count
    // Callback function cannot return a result, should have return type void
    // TODO
}

Callback::Callback(const Callback &c) {
    module = c.module;
    topic = c.topic;
    table_index = c.table_index;
}
