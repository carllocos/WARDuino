#pragma once
#include <cstdint>
#include <functional>
#include <stack>
#include <unordered_map>

#include "../Instrumentation/schedule.h"
#include "../Instrumentation/timestamp.h"
#include "../Utils/sockets.h"
#include "../WARDuino/structs.h"
#include "./hook.h"
#include "./instrumentation_structs.h"

#define INSTRUMENTATION_INTERCEPT_OPCODE 0xff

typedef struct MonitoredFrame {
    uint32_t addr{0};
    int frame_idx{-1};
} MonitoredFrame;

class InstrumentationManager {
   private:
    Channel *fun_call_channel{};

    std::stack<MonitoredFrame> frames_to_monitor{};

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

    bool run_hook(
        const Channel &output, Module &module, uint32_t local_fidx, Hook &hook,
        std::function<void(std::function<void()>)> sendSubscriptionMsg,
        RunningState &runningState);

    bool do_value_substitution(Module *module, uint32_t func_called,
                               Hook *hook);

    InstrumentationPrimitiveFunc *start_primitive_call_interception(
        Module &m, uint32_t target_func);

    InstrumentationWasmAddr *start_wasm_addr_intercept(
        Module &module, const uint32_t addr, const InstrumentMoment moment);

    bool do_before_wasm_addr_hooks(const Channel &hookOutput, Module &module,
                                   TimeStamp &currentTime, uint32_t addr,
                                   uint8_t &opcode, RunningState &runningState);

   public:
    bool awakeOnNextInstruction = false;

    InstrumentationManager();

    bool addAroundFunctionHook(Module &m, uint32_t func_idx,
                               const Hook &around);

    bool has_AroundFunction(uint32_t funID);

    bool has_HookOnWasmAddr(uint32_t addr, InstrumentMoment moment);

    bool isAddHookAllowed(uint32_t funID);

    bool apply_primitive_call_instrumentation(const Channel &hookOutput,
                                              Module *module,
                                              TimeStamp *currentTime,
                                              RunningState &runningState);

    bool apply_wasm_addr_instrumentation(const Channel &output, Module *module,
                                         TimeStamp *currentTime,
                                         uint8_t &opcode,
                                         RunningState &runningState);

    void apply_instrumentation_after_instr(const Channel &hookOutput,
                                           Module *module,
                                           RunningState &runningState);

    void registerAroundFunctionChannel(Channel *channel);

    bool addHookOnOnWasmAddress(Module &module, uint32_t addr, Hook &hook,
                                const InstrumentMoment moment);

    bool removeHooksOnWasmAddress(Module &module, uint32_t addr,
                                  const InstrumentMoment moment);
};

bool Instrumentation_interceptPrimitiveCall(Module *module);