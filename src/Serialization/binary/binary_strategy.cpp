#include "binary_strategy.h"

#include "binary_breakpoints.h"
#include "binary_callback_mapping.h"
#include "binary_callstack.h"
#include "binary_event.h"
#include "binary_frame.h"
#include "binary_functions.h"
#include "binary_globals.h"
#include "binary_memory.h"
#include "binary_multiple_objects.h"
#include "binary_pc.h"
#include "binary_stack.h"
#include "binary_stackvalue.h"
#include "binary_table.h"
#include "json_br_table.h"

void JSON_initialize_serializer(SerializationStrategy& strategy) {
    strategy.startMultipleStructsSerialization =
        &JSON_startMultipleStructsSerialization;
    strategy.endMultipleStructsSerialization =
        &JSON_endMultipleStructsSerialization;

    strategy.serializePC = &JSON_serialize_PC;
    strategy.serializeBreakpoints = &JSON_serialize_Breakpoints;
    strategy.serializeFunctions = &JSON_serialize_Functions;
    strategy.serializeFrame = &JSON_serialize_Frame;
    strategy.serializeCallstack = &JSON_serialize_Callstack;
    strategy.serializeStackValue = &JSON_serialize_StackValue;
    strategy.serializeStack = &JSON_serialize_Stack;
    strategy.serializeGlobals = &JSON_serialize_Globals;
    strategy.serializeEvent = &JSON_serialize_Event;
    strategy.serializeEvents = &JSON_serialize_Events;
    strategy.serializeCallbackMapping = &JSON_serialize_CallbackMapping;
    strategy.serializeTable = &JSON_serialize_Table;
    strategy.serializeBranchingTable = &JSON_serialize_BranchingTable;
    strategy.serializeMemory = &JSON_serialize_Memory;
    strategy.serializeCallbackMappings = &JSON_serialize_CallbackMappings;
    strategy.serializeCallbackMappingsVers2 =
        &JSON_serialize_CallbackMappingsVers2;
}

void JSON_initialize_deserializer(SerializationStrategy& strategy) {
    strategy.deserializeStackValue = &JSON_deserialize_StackValue;
    strategy.deserializeFrame = &JSON_deserialize_Frame;
    strategy.deserializePC = &JSON_deserialize_PC;
    strategy.deserializeBreakpoints = &JSON_deserialize_Breakpoints;
    strategy.deserializeCallstack = &JSON_deserialize_Callstack;
    strategy.deserializeStack = &JSON_deserialize_Stack;
    strategy.deserializeGlobals = &JSON_deserialize_Globals;
    strategy.deserializeEvent = &JSON_deserialize_Event;
    strategy.deserializeEvents = &JSON_deserialize_Events;
    strategy.deserializeCallbackMapping = &JSON_deserialize_CallbackMapping;
    strategy.deserializeTable = &JSON_deserialize_Table;
    strategy.deserializeBranchingTable = &JSON_deserialize_BranchingTable;
    strategy.deserializeMemory = &JSON_deserialize_Memory;
    strategy.deserializeCallbackMappings = &JSON_deserialize_CallbackMappings;
    strategy.deserializeCallbackMappingsVers2 =
        &JSON_deserialize_CallbackMappingsVers2;
}

void JSON_initialize_strategy(SerializationStrategy& strategy) {
    JSON_initialize_serializer(strategy);
    JSON_initialize_deserializer(strategy);
}
