#pragma once
#include <cstdint>
#include <functional>
#include <stack>
#include <unordered_map>
#include <vector>

#include "../Instrumentation/logical_clock.h"
#include "../Instrumentation/schedule.h"
#include "../Interrupts/interrupt_hook_on_event_struct.h"
#include "../Utils/sockets.h"
#include "../WARDuino/CallbackHandler.h"
#include "../WARDuino/event_structs.h"
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
    LogicalClock lastObservedTime{};

    Channel *fun_call_channel{};

    std::stack<MonitoredFrame> frames_to_monitor{};

    std::unordered_map<uint32_t, InstrumentationPrimitiveFunc *>
        instr_primitive_funcs{};

    std::unordered_map<uint32_t, InstrumentationWasmAddr *>
        instr_wasm_addr_before{};

    std::unordered_map<uint32_t, InstrumentationWasmAddr *>
        instr_wasm_addr_after{};

    Hook *hooksForOnNewEvent{};

    Hook *hooksForOnEventHandling{};

    Hook *hooksForOnError{};

    InstrumentationPrimitiveFunc *new_Primitive_Instrumentation();

    InstrumentationWasmAddr *new_WasmAddress_Instrumentation();

    bool do_remote_call(Channel &channel, Module *m, uint32_t local_fidx,
                        uint32_t func_to_call, bool isProxyCall);

    // TODO refactor to use struct to pass optional arguments e.g. local_fix
    bool run_hook(
        const Channel &output, Module &module, uint32_t local_fidx, Hook &hook,
        std::function<void(std::function<void()>)> sendSubscriptionMsg,
        RunningState &runningState);

    bool run_hook_event(
        const Channel &output, Module &module, Hook &hook,
        std::function<void(std::function<void()>)> sendSubscriptionMsg,
        Event *ev, HookEventMoment hookMoment);

    void run_hook_on_new_event(const Channel &output, Module &module,
                               Hook &hook, Event *ev);

    void run_hook_on_handled_event(const Channel &output, Module &module,
                                   Hook &hook, Event *ev);

    bool do_value_substitution(Module *module, uint32_t func_called,
                               Hook *hook);

    InstrumentationPrimitiveFunc *start_primitive_call_interception(
        Module &m, uint32_t target_func);

    InstrumentationWasmAddr *start_wasm_addr_intercept(
        Module &module, const uint32_t addr, const InstrumentMoment moment);

    bool do_before_wasm_addr_hooks(const Channel &hookOutput, Module &module,
                                   LogicalClock &currentTime, uint32_t addr,
                                   uint8_t &opcode, RunningState &runningState);

    /*
     * Methods that stop instrumentation
     */

    void stopRunningHooksOnNewEvents();

    void stopRunningHooksOnEventsHandled();

    void stopRunningHooksOnError();

   public:
    bool awakeOnNextInstruction = false;
    bool interceptEvents = false;
    bool interceptError = false;

    InstrumentationManager();

    void registerAroundFunctionChannel(Channel *channel);

    /*
     * Hook registration methods
     */

    bool addAroundFunctionHook(Module &m, uint32_t func_idx,
                               const Hook &around);

    bool addHookOnWasmAddress(Module &module, uint32_t addr, Hook &hook,
                              const InstrumentMoment moment);

    bool removeHooksOnWasmAddress(Module &module, uint32_t addr,
                                  const InstrumentMoment moment);

    bool addHookOnNewEvent(Hook &hook);

    bool addHookOnEventHandling(Hook &hook);

    bool addHookOnError(Hook &hook);

    /*
     *  Predicate methods
     */
    bool has_AroundFunction(uint32_t funID);

    bool has_HookOnWasmAddr(uint32_t addr, InstrumentMoment moment);

    bool isAddHookAllowed(uint32_t funID);

    bool isAddHookOnEventAllowed(Hook &hook);

    bool isAddHookOnEventHandlingAllowed(Hook &hook);

    bool isAddHookOnErrorAllowed(Hook &hook);

    /*
     * Running hooks methods
     */

    /* method called by the CallbackHandler to give control the Instrumentration
     * object for intercepting events that will be handled.
     *
     * returns bool :  if true the Instrumentation wants to continue handling
     * intercepting events after the call false the. If false control is given
     * back to the callback handler
     */
    bool runHookForOnEventHandling(const Channel &output, Module *module);

    void runHooksForOnNewEvent(const Channel &output, Module *module);

    void runHooksOnError(const Channel &output, Module *module,
                         LogicalClock *currentTime);

    void runHooksAfterWasmAddr(const Channel &output, Module *module,
                               RunningState &runningState);

    bool runHooksOnInterceptedFuncCall(const Channel &output, Module *module,
                                       LogicalClock *currentTime,
                                       RunningState &runningState);

    bool runHooksOnWasmAddr(const Channel &output, Module *module,
                            LogicalClock *currentTime, uint8_t &opcode,
                            RunningState &runningState);
};

bool Instrumentation_interceptPrimitiveCall(Module *module);