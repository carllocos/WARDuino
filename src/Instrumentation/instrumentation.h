#pragma once
#include <cstdint>
#include <unordered_map>

#include "../Instrumentation/schedule.h"
#include "../Instrumentation/timestamp.h"
#include "../Utils/sockets.h"
#include "../WARDuino/structs.h"
#include "./action.h"
#define INSTRUMENTATION_INTERCEPT_OPCODE 0xff

enum InstrumentMoment {
    InstrumentBefore = 0x01,
    InstrumentAfter = 0x02,
};

typedef struct InstrumentationPrimitiveFunc {
    uint32_t func_idx;  // func for which the around action is registered
    Primitive original_func{};  // original function that is restored
    Action *action{};           // action to perform instead of original_func
} InstrumentationPrimitiveFunc;

typedef struct {
    uint32_t address{};         // wasm address that needs to be intercepted
    uint8_t original_opcode{};  // original opcode
    Action *action{};           // actions to perform on address
} InstrumentationWasmAddr;

class InstrumentationManager {
   private:
    Channel *fun_call_channel{};

    std::unordered_map<uint32_t, InstrumentationPrimitiveFunc *>
        instr_primitive_funcs{};

    std::unordered_map<uint32_t, InstrumentationWasmAddr *>
        instr_wasm_addr_before{};

    std::unordered_map<uint32_t, InstrumentationWasmAddr *>
        instr_wasm_addr_after{};

    InstrumentationPrimitiveFunc *new_Primitive_Instrumentation();

    InstrumentationWasmAddr *new_WasmAddress_Instrumentation();

    void remove_completed_action(InstrumentationPrimitiveFunc *inst,
                                 Action *action_completed);

    bool do_remote_call(Channel &channel, Module *m,
                        InstrumentationPrimitiveFunc *instr);

    bool do_value_substitution(Module *module, uint32_t func_called,
                               Action *action);

    InstrumentationPrimitiveFunc *start_primitive_call_interception(
        Module &m, uint32_t target_func);

    InstrumentationWasmAddr *start_wasm_addr_intercept(
        Module &module, const uint32_t addr, const InstrumentMoment moment);

   public:
    InstrumentationManager();

    bool addAroundFunctionAction(Module &m, uint32_t func_idx,
                                 const Action &around);

    bool has_AroundFunction(uint32_t funID);

    bool has_ActionOnWasmAddr(uint32_t addr, InstrumentMoment moment);

    bool isAddActionAllowed(uint32_t funID);

    bool apply_primitive_call_instrumentation(Module *module,
                                              TimeStamp *currentTime);

    bool apply_wasm_addr_instrumentation(Module *module,
                                         TimeStamp *currentTime);

    void registerAroundFunctionChannel(Channel *channel);

    bool addActionOnWasmAddress(Module &module, uint32_t addr, Action &action,
                                const InstrumentMoment moment);
};

bool Instrumentation_interceptPrimitiveCall(Module *module);