#pragma once
#include <cstdint>
#include <set>

#include "../Structs/struct_block.h"
#include "../Structs/struct_frame.h"
#include "../Structs/struct_memory.h"
#include "../Structs/struct_stackvalue.h"
#include "../Structs/struct_table.h"
#include "../Utils/sockets.h"
#include "../WARDuino/CallbackHandler.h"
#include "deserialized_structs.h"

typedef struct {
    void (*startMultipleStructsSerialization)(const Channel& channel);
    void (*endMultipleStructsSerialization)(const Channel& channel);

    void (*serializePC)(const Channel& channel, const uint8_t* pc,
                        const Module* m);
    uint32_t (*deserializePC)(uint8_t* src);

    void (*serializeBreakpoints)(const Channel& channel,
                                 const std::set<uint8_t*>& bps,
                                 const Module* mod);
    BreakpointDeserialized (*deserializeBreakpoints)(uint8_t* src);

    void (*serializeFunctions)(const Channel& channel, const Block* functions,
                               uint32_t begin, uint32_t end, const Module* mod);

    void (*serializeFrame)(const Channel& channel, const Frame* frame,
                           int frame_index, const Module* mod);
    void (*deserializeFrame)(uint8_t* src, Frame* dest_frame);

    void (*serializeCallstack)(const Channel& channel, const Frame* callstack,
                               int begin, int end, const Module* m);
    FrameDeserialized (*deserializeCallstack)(uint8_t* src);

    void (*serializeStackValue)(const Channel& channel, const StackValue& value,
                                uint32_t value_idx);
    void (*deserializeStackValue)(uint8_t* src, StackValue* dest_value);

    void (*serializeStack)(const Channel& channel, const StackValue* stack,
                           int begin, int end);
    StackDeserialized (*deserializeStack)(uint8_t* src);

    void (*serializeGlobals)(const Channel& channel, const StackValue* globals,
                             uint32_t begin, uint32_t end);
    GlobalsDeserialized (*deserializeGlobals)(uint8_t* src);

    void (*serializeEvent)(const Channel& channel, const Event& event);
    void (*deserializeEvent)(uint8_t* src, Event& src_event);
    void (*serializeEvents)(const Channel& channel,
                            std::deque<Event>::const_iterator start,
                            std::deque<Event>::const_iterator end);
    EventsDeserialized (*deserializeEvents)(uint8_t* src);

    void (*serializeCallbackMapping)(const Channel& channel, std::string topic,
                                     std::vector<Callback>* cbs);
    void (*deserializeCallbackMapping)(uint8_t* src, std::string* dest_id,
                                       Callback* dest_cb);

    void (*serializeCallbackMappings)(const Channel& channel,
                                      CallbackHandler::callbackIterator start,
                                      CallbackHandler::callbackIterator end);
    std::unordered_map<std::string, std::vector<Callback>>* (
        *deserializeCallbackMappings)(uint8_t* src);

    void (*serializeTable)(const Channel& channel, const Table& table);
    void (*deserializeTable)(uint8_t* src, const Table& table);

    void (*serializeBranchingTable)(const Channel& channel,
                                    const uint32_t* br_table);
    void (*deserializeBranchingTable)(uint8_t* src, uint32_t* br_table);

    void (*serializeMemory)(const Channel& channel, const Memory& memory);
    void (*deserializeMemory)(uint8_t* src, Memory* memory_dest);

    // TODO remove: safe to remove CallbackMappingsVers2 when plugin and vm no
    // longer uses vers2
    void (*serializeCallbackMappingsVers2)(
        const Channel& channel, CallbackHandler::callbackIterator start,
        CallbackHandler::callbackIterator end);
    std::unordered_map<std::string, std::vector<Callback>>* (
        *deserializeCallbackMappingsVers2)(uint8_t* src);

} SerializationStrategy;