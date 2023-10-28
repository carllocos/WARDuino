#pragma once
#include <cstdint>
#include <functional>
#include <unordered_map>

#include "../Instrumentation/schedule.h"
#include "../Instrumentation/timestamp.h"
#include "../Utils/sockets.h"
#include "../WARDuino/structs.h"
#include "./action.h"
#include "./instrumentation_structs.h"

#define INSTRUMENTATION_INTERCEPT_OPCODE 0xff

class InstrumentationManager {
   private:
    Channel *fun_call_channel{};

    uint32_t addr_yet_to_finish{};

    std::unordered_map<uint32_t, InstrumentationPrimitiveFunc *>
        instr_primitive_funcs{};

    std::unordered_map<uint32_t, InstrumentationWasmAddr *>
        instr_wasm_addr_before{};

    std::unordered_map<uint32_t, InstrumentationWasmAddr *>
        instr_wasm_addr_after{};

    InstrumentationPrimitiveFunc *new_Primitive_Instrumentation();

    InstrumentationWasmAddr *new_WasmAddress_Instrumentation();

    bool do_remote_call(Channel &channel, Module *m, uint32_t local_fidx,
                        uint32_t func_to_call);

    bool run_action(
        const Channel &output, Module &module, uint32_t local_fidx,
        Action &action,
        std::function<void(std::function<void()>)> sendSubscriptionMsg);

    bool do_value_substitution(Module *module, uint32_t func_called,
                               Action *action);

    InstrumentationPrimitiveFunc *start_primitive_call_interception(
        Module &m, uint32_t target_func);

    InstrumentationWasmAddr *start_wasm_addr_intercept(
        Module &module, const uint32_t addr, const InstrumentMoment moment);

    bool do_before_wasm_addr_actions(const Channel &output, Module &module,
                                     TimeStamp &currentTime, uint32_t addr,
                                     uint8_t &opcode);

   public:
    bool awakeOnNextInstruction = false;

    InstrumentationManager();

    bool addAroundFunctionAction(Module &m, uint32_t func_idx,
                                 const Action &around);

    bool has_AroundFunction(uint32_t funID);

    bool has_ActionOnWasmAddr(uint32_t addr, InstrumentMoment moment);

    bool isAddActionAllowed(uint32_t funID);

    bool apply_primitive_call_instrumentation(const Channel &ouput,
                                              Module *module,
                                              TimeStamp *currentTime);

    bool apply_wasm_addr_instrumentation(const Channel &output, Module *module,
                                         TimeStamp *currentTime,
                                         uint8_t &opcode);

    void apply_instrumentation_after_instr(const Channel &output,
                                           Module *module);

    void registerAroundFunctionChannel(Channel *channel);

    bool addActionOnWasmAddress(Module &module, uint32_t addr, Action &action,
                                const InstrumentMoment moment);
};

bool Instrumentation_interceptPrimitiveCall(Module *module);