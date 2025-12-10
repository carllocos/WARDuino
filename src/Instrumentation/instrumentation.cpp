#include "instrumentation.h"

#include <cstddef>
#include <cstdint>

#include "../Interrupts/interrupt_hook_on_addr.h"
#include "../Interrupts/interrupt_hook_on_error.h"
#include "../Interrupts/interrupt_hook_on_event.h"
#include "../Interrupts/interrupt_remote_call.h"
#include "../Interrupts/interrupt_response.h"
#include "../Utils/macros.h"
#include "../WARDuino/vm_exception.h"
#include "hook.h"
#include "instrumentation_structs.h"
#include "logical_clock.h"
#include "schedule.h"

InstrumentationManager::InstrumentationManager() {}

void InstrumentationManager::registerAroundFunctionChannel(Channel *channel) {
    this->fun_call_channel = channel;
    this->lastObservedTime.nr_of_events = 0;
    this->lastObservedTime.nr_of_instructions =
        30;  // not set to 0 so the first instr can also be hooked upon
}

HooksPrimitiveFunc *InstrumentationManager::new_Primitive_Instrumentation() {
    return new HooksPrimitiveFunc{};
}

void InstrumentationManager::delete_Primitive_Instrumentation(
    HooksPrimitiveFunc *func) {
    Hooks_free_hooks(func->hook);
    func->original_func = nullptr;
    delete func;
}

HooksWasmAddr *InstrumentationManager::new_WasmAddress_Instrumentation() {
    return new HooksWasmAddr{};
}

bool InstrumentationManager::has_HooksOnAroundFunction(uint32_t funID) {
    return this->hooks_around_prim_funcs.count(funID) > 0;
}

bool InstrumentationManager::has_HookOnWasmAddr(uint32_t addr,
                                                HookMoment moment) {
    switch (moment) {
        case HookBefore:
            return this->instr_wasm_addr_before.count(addr) > 0;
        case HookAfter:
            return this->instr_wasm_addr_after.count(addr) > 0;
        default:
            FATAL("Around monitor not supported");
    }
    return false;
}

bool InstrumentationManager::isAddHookOnEventAllowed(Hook &hook) {
    switch (hook.kind) {
        case EventInspect:
        case EventRemove:
        case StateInspect:
            return true;
        default:
            return false;
    }
}

bool InstrumentationManager::isAddHookOnEventHandlingAllowed(Hook &hook) {
    return this->isAddHookOnEventAllowed(hook);
}

bool InstrumentationManager::isAddHookAroundFuncAllowed(uint32_t funID) {
    if (!this->has_HooksOnAroundFunction(funID)) return true;
    // Dissallows hooks that have been scheduled for always if one is
    // alread in place

    printf(
        "TODO: addHookAroundFunction yet to decide what to do in case of "
        "multiple hooks");

    // TODO: decide whether even if you have always adding this is allowed
    // because it just replaces it or becomes another hook. if the hook kind
    // is always tou replace the old one with the new one if the hook is
    // something else than always such as once then that hook is put in front
    // of the always so that once the once is consumed the always remains.
    Hook *hook = this->hooks_around_prim_funcs[funID]->hook;
    return hook == nullptr || hook->schedule.kind != ScheduleAlways;
}

bool InstrumentationManager::isAddHookOnErrorAllowed(Hook &hook) {
    return hook.kind == StateInspect;
}

bool InstrumentationManager::addHookAroundFunction(Module &m, uint32_t func_idx,
                                                   const Hook &around) {
    if (func_idx > m.function_count) {
        return false;
    } else if (func_idx < m.import_count) {
        HooksPrimitiveFunc *instr =
            this->start_primitive_call_interception(m, func_idx);
        Hook *cpy{};
        if (instr == nullptr || (cpy = Hooks_copyHook(around)) == nullptr) {
            return false;
        }
        instr->hook = Hooks_add_and_sort(instr->hook, cpy);
        return true;
    } else {
        printf("TODO: addHookAroundFunction for non primitive functions\n");
        return false;
    }
}

bool InstrumentationManager::removeHooksAroundFunction(Module &m,
                                                       uint32_t func_idx) {
    return this->stop_primitive_call_interception(m, func_idx);
}

bool InstrumentationManager::removeHooksOnWasmAddress(Module &module,
                                                      uint32_t addr,
                                                      const HookMoment moment) {
    if (!isToPhysicalAddrPossible(addr, &module)) {
        // address is not in module
        return false;
    }
    if (!this->has_HookOnWasmAddr(addr, moment)) {
        return false;
    }

    HooksWasmAddr *instr;
    std::unordered_map<uint32_t, HooksWasmAddr *> *mapForErase;
    bool restoreOpcode;
    if (moment == HookBefore) {
        instr = instr_wasm_addr_before[addr];
        mapForErase = &this->instr_wasm_addr_before;
        restoreOpcode = !this->has_HookOnWasmAddr(addr, HookAfter);
    } else {
        instr = instr_wasm_addr_after[addr];
        mapForErase = &this->instr_wasm_addr_after;
        restoreOpcode = !this->has_HookOnWasmAddr(addr, HookBefore);
    }

    while (instr->hook != nullptr) {
        Hook *hookToFree = instr->hook;
        instr->hook = instr->hook->nextHook;
        Hooks_free_hook(hookToFree);
    }

    if (restoreOpcode) {
        module.bytes[addr] = instr->original_opcode;
    }

    mapForErase->erase(addr);
    delete instr;
    return true;
}

bool InstrumentationManager::addHookOnWasmAddress(Module &module, uint32_t addr,
                                                  Hook &hook,
                                                  const HookMoment moment) {
    if (!isToPhysicalAddrPossible(addr, &module)) {
        // address is not in module
        return false;
    }
    HooksWasmAddr *instr =
        this->start_wasm_addr_intercept(module, addr, moment);

    Hook *cpy{};
    if (instr == nullptr || (cpy = Hooks_copyHook(hook)) == nullptr) {
        return false;
    }
    instr->hook = Hooks_add_and_sort(instr->hook, cpy);
    return true;
}

bool InstrumentationManager::addHookOnNewEvent(Hook &hook) {
    if (!this->isAddHookOnEventAllowed(hook)) {
        return false;
    }
    CallbackHandler::pendingEventsActivated = true;
    Hook *h = new Hook();
    *h = hook;
    h->nextHook = nullptr;
    this->hooksForOnNewEvent = Hooks_add_and_sort(this->hooksForOnNewEvent, h);

    return true;
}

bool InstrumentationManager::addHookOnEventHandling(Hook &hook) {
    if (!this->isAddHookOnEventHandlingAllowed(hook)) {
        return false;
    }

    Hook *h = new Hook();
    *h = hook;
    h->nextHook = nullptr;
    this->hooksForOnEventHandling =
        Hooks_add_and_sort(this->hooksForOnEventHandling, h);
    this->interceptEvents = true;

    return true;
}

bool InstrumentationManager::addHookOnError(Hook &hook) {
    if (!this->isAddHookOnErrorAllowed(hook)) {
        return false;
    }

    Hook *h = new Hook();
    *h = hook;
    h->nextHook = nullptr;
    this->hooksForOnError = Hooks_add_and_sort(this->hooksForOnError, h);
    this->interceptError = true;

    return true;
}

void Interrupt_RemoteCall_free_response(FunCallResponse &response) {
    if (response.result != nullptr) {
        if (response.result->value != nullptr) {
            delete response.result->value;
        }
        if (response.result->exception_msg != nullptr) {
            delete response.result->exception_msg;
        }
    }
}

bool InstrumentationManager::do_remote_call(Channel &channel, Module *module,
                                            uint32_t local_fidx,
                                            uint32_t func_to_call,
                                            bool isProxyCall) {
    if (this->fun_call_channel == nullptr) {
        VM_Exception_write("No channel set to perform around function call");
        return false;
    }

    // get args
    Type *func_type = module->functions[local_fidx].type;
    StackValue *args = nullptr;
    if (func_type->param_count > 0) {
        module->sp -= func_type->param_count;  // pop args
        args = module->stack + module->sp + 1;
    }

    FunCallResponse response;
    Interrupt_RemoteCall_call(func_to_call, args, func_type->param_count,
                              channel, &response, isProxyCall);
    if (response.type == INTERRUPT_RESPONSE_TYPE_ERROR) {
        printf("TODO copy error properly\n");

        printf("Remotecall failed error_code%" PRIu8 " \n",
               response.error_code);
        VM_Exception_write("Remotecall failed error_code%" PRIu8 "",
                           response.error_code);
        return false;
    }

    printf(
        "TODO: check if local fun expects a result and then on "
        "stack\n");

    // Push result on stack
    CallResult *result = response.result;
    if (result->value != nullptr) {
        module->sp += 1;
        StackValue *value = &module->stack[module->sp];
        value->value_type = result->value->value_type;
        value->value = result->value->value;
    } else if (result->exception_msg != nullptr) {
        VM_Exception_write("%s", result->exception_msg);
    }
    bool success = result->success;
    Interrupt_RemoteCall_free_response(response);
    return success;
}

bool InstrumentationManager::do_value_substitution(Module *module,
                                                   uint32_t func_called,
                                                   Hook *hook) {
    Type *type = module->functions[func_called].type;
    module->sp -= type->param_count;  // pop args
    if (type->result_count > 0) {
        if (hook->value.result == nullptr) {
            VM_Exception_write("No Substitute value provided");
            return false;
        }
        module->sp += 1;
        StackValue *value = &module->stack[module->sp];
        value->value_type = hook->value.result->value_type;
        value->value = hook->value.result->value;
    }
    return true;
}

bool InstrumentationManager::runHooksOnInterceptedFuncCall(
    const Channel &output, Module *module, LogicalClock *currentTime,
    RunningState &runningState) {
    uint32_t primitive_called = module->last_called;

    // Before call instrumentation

    // Around call instrumentation(s)

    auto iterator = hooks_around_prim_funcs.find(primitive_called);
    if (iterator == hooks_around_prim_funcs.end() ||
        iterator->second->hook == nullptr) {
        VM_Exception_write(
            "No Instrumentation registered for primitive %" PRIu32 "",
            primitive_called);
        // TODO ADD subscription message for no instrumentation registered
        return false;
    }

    HooksPrimitiveFunc *instr = iterator->second;
    Hook *hookToRun = Hooks_nextScheduledHook(instr->hook, *currentTime);
    if (hookToRun == nullptr) {
        // We did not find a hook to run, but we may have to wait for an
        // event to occur
        if (Hooks_isHookWaitingForEvent(instr->hook, *currentTime)) {
            printf(
                "TODO: restore the PC to the position before the call "
                "(and pause the VM)?");
            WARDuino::instance()->program_state = WARDUINOpause;
            // TODO send messages via subscription for waiting for incoming
            // event to occur
            return true;
        } else {
            VM_Exception_write("No hook scheduled for primitive call");
            return false;
        }
    }

    auto printSubMsg = [](std::function<void()> hookOutput) {};
    bool around_successful =
        this->run_hook(output, *module, primitive_called, *hookToRun,
                       printSubMsg, runningState);
    HooksRemoveResult res;
    Hooks_remove_completed_hook(res, instr->hook, hookToRun);
    instr->hook = res.newList;

    // After call instrumentation(s)
    return around_successful;
}

bool InstrumentationManager::run_hook_event(
    const Channel &output, Module &module, Hook &hook,
    std::function<void(std::function<void()>)> sendSubscriptionMsg, Event *ev,
    HookEventMoment hookMoment) {


HookRunResult InstrumentationManager::run_hook(const Channel &output,
                                               Module &module, Hook &hook,
                                               HookArgs &args) {
    // TODO throw error if a hook is encountered that cannot be scheduled
    // TODO check if pauzing, running still works
    bool do_run = Hook_isScheduledForNow(args.currentTime, hook.schedule);
    if (!do_run) {
        return HookDelayed;
    }
    bool hook_success = false;
    // TODO integrate hook removal here i.e., onto result
    // while (hookToRun != nullptr) {
    //     this->run_hook_on_new_event(output, *module, *hookToRun, ev);
    //     Hooks_remove_completed_hook(res, this->hooksForOnNewEvent,
    //                                 hookToRun);
    //     this->hooksForOnNewEvent = res.newList;
    //     if (CallbackHandler::pendingEvents->empty() ||
    //         ev != CallbackHandler::pendingEvents->front()) {
    //         // event got removed by last hook
    //         // do no run remaining hooks
    //         break;
    //     }
    //     hookToRun = res.nextHook;
    // }
    switch (hook.kind) {
        case ProxyCall:
        case RemoteCall:
            hook_success = this->do_remote_call(
                *this->fun_call_channel, &module, args.local_fidx,
                hook.value.target_fidx, hook.kind == ProxyCall);
            break;
        case ValueSubstitution:
            hook_success =
                this->do_value_substitution(&module, args.local_fidx, &hook);
            break;
        case StateInspect: {
            const Module *m = &module;
            bool success = true;
            std::function<void()> printState = [&output, m, hook, &success]() {
                bool includeHeader = false;
                bool includeNewline = false;
                success = Interrupt_Inspect_inspect_json_output(
                    output, m, *hook.value.state, includeHeader,
                    includeNewline);
            };
            args.sendSubscriptionMsg(printState);
            if (!success) {
                VM_Exception_write("Unexisting State to inspect");
            }
            hook_success = success;
            break;
        }
        case ChangeRunningState:
            args.runningState = hook.value.runState;
            hook_success = true;
            break;
        case EventAdd: {
            CallbackHandler::push_event(hook.value.ev->topic,
                                        hook.value.ev->payload,
                                        hook.value.ev->payload_length);
            hook_success = true;
            break;
        }
        case EventRemove:
            if (args.eventMoment == HookOnNewEvent) {
                if (!CallbackHandler::pendingEvents->empty()) {
                    CallbackHandler::pendingEvents->pop_front();
                }
                hook_success = true;
            } else if (args.eventMoment == HookOnEventHandling) {
                if (!CallbackHandler::events->empty()) {
                    CallbackHandler::events->pop_front();
                }
                hook_success = true;
            } else {
                VM_Exception_write("Unsupported event hook moment");
                hook_success = false;
            }
            break;
        case EventInspect:
            Interrupt_HookOnEvent_send_Binary_subscribe_message(output,
                                                                *(args.ev));
            hook_success = true;
            break;
        default:
            VM_Exception_write("About to run an unsupported hook kind");
            hook_success = false;
            break;
    }

void InstrumentationManager::run_hook_on_new_event(const Channel &output,
                                                   Module &module, Hook &hook,
                                                   Event *ev) {
    auto printSubMsg = [&output](std::function<void()> hookOutput) {
        Interrupt_HookOnEvent_send_JSON_subscribe_message(
            output, HookOnNewEvent, hookOutput);
    };
    this->run_hook_event(output, module, hook, printSubMsg, ev, HookOnNewEvent);
}

void InstrumentationManager::run_hook_on_handled_event(const Channel &output,
                                                       Module &module,
                                                       Hook &hook, Event *ev) {
    auto printSubMsg = [&output](std::function<void()> hookOutput) {
        Interrupt_HookOnEvent_send_JSON_subscribe_message(
            output, HookOnEventHandling, hookOutput);
    };
    this->run_hook_event(output, module, hook, printSubMsg, ev,
                         HookOnEventHandling);
}

void InstrumentationManager::runHooksOnError(const Channel &output,
                                             Module *module,
                                             LogicalClock *currentTime) {
    if (this->hooksForOnError == nullptr) {
        this->stopRunningHooksOnError();
        return;
    }

    auto printSubMsg = [&output](std::function<void()> hookOutput) {
        Interrupt_HookOnError_send_JSON_subscribe_message(output, hookOutput);
    };

    RunningState unusedState = WARDUINOpause;
    Hook *hooks = this->hooksForOnError;
    HooksRemoveResult res;
    while (hooks != nullptr) {
        this->run_hook(output, *module, 0, *hooks, printSubMsg, unusedState);
        Hooks_remove_completed_hook(res, this->hooksForOnError, hooks);
        this->hooksForOnError = res.newList;
        hooks = res.nextHook;
    }
}

void InstrumentationManager::runHooksAfterWasmAddr(const Channel &output,
                                                   Module *module,
                                                   RunningState &runningState) {
    // Only called when tool client wants to do something after some wasm
    // addr. Inefficiently called after each instruction execution.
    // Benchmark needed to determine whether an alternative approach is
    // required

    while (!this->frames_to_monitor.empty()) {
        MonitoredFrame frame = this->frames_to_monitor.top();
        if (frame.frame_idx < module->csp) {
            break;
        }

        this->frames_to_monitor.pop();

        uint32_t addr = frame.addr;
        if (!has_HookOnWasmAddr(addr, HookAfter)) {
            continue;
        }
        HooksWasmAddr *instr = this->instr_wasm_addr_after[addr];
        auto printSubMsg = [&output, addr](std::function<void()> hookOutput) {
            Interrupt_HookOnAddr_send_JSON_subscribe_message(output, HookAfter,
                                                             addr, hookOutput);
        };

        Hook *hooks = instr->hook;
        HooksRemoveResult res;
        while (hooks != nullptr) {
            this->run_hook(output, *module, 0, *hooks, printSubMsg,
                           runningState);
            Hooks_remove_completed_hook(res, instr->hook, hooks);
            instr->hook = res.newList;
            hooks = res.nextHook;
        }
    }

    if (this->frames_to_monitor.empty()) {
        this->awakeOnNextInstruction = false;
    }
}

bool InstrumentationManager::runHooksOnWasmAddr(const Channel &output,
                                                Module *module,
                                                LogicalClock *currentTime,
                                                uint8_t &opcode,
                                                RunningState &runningState) {
    module->pc_ptr -= 1;  // set pc to start of instruction
    uint32_t addr = toVirtualAddress(module->pc_ptr, module);
    bool success = true;
    bool upcodeRestored = false;

    if (LogicalClock_is_t1_equal_t2(this->lastObservedTime, *currentTime)) {
        // Reentering an addr for which the hooks were just run
        // do not run the hooks but advance computation
        auto instr = this->has_HookOnWasmAddr(addr, HookBefore)
                         ? this->instr_wasm_addr_before[addr]
                         : this->instr_wasm_addr_after[addr];
        opcode = instr->original_opcode;
        module->pc_ptr += 1;
    } else {
        this->lastObservedTime = *currentTime;
        if (this->has_HookOnWasmAddr(addr, HookBefore)) {
            success = this->do_before_wasm_addr_hooks(
                output, *module, *currentTime, addr, opcode, runningState);
            upcodeRestored = true;
        }

        if (runningState != WARDUINOpause) {
            module->pc_ptr += 1;
        }

        if (this->has_HookOnWasmAddr(addr, HookAfter)) {
            // save frame and addr for after addr hooks
            MonitoredFrame frame{};
            frame.addr = addr;
            frame.frame_idx = module->csp;
            this->frames_to_monitor.push(frame);
            this->awakeOnNextInstruction = true;
            if (!upcodeRestored) {
                // When there is no before the opcode needs to be set
                auto instr = this->instr_wasm_addr_after[addr];
                opcode = instr->original_opcode;
            }
        }
    }
    return success;
}

bool InstrumentationManager::do_before_wasm_addr_hooks(
    const Channel &output, Module &module, LogicalClock &currentTime,
    uint32_t addr, uint8_t &opcode, RunningState &runningState) {
    if (!has_HookOnWasmAddr(addr, HookBefore)) {
        VM_Exception_write(
            "No hook registered on instrumented addr %" PRIu32 "", addr);
        return false;
    }
    HooksWasmAddr *instr = this->instr_wasm_addr_before[addr];

    auto printSubMsg = [&output, addr](std::function<void()> hookOutput) {
        Interrupt_HookOnAddr_send_JSON_subscribe_message(output, HookBefore,
                                                         addr, hookOutput);
    };
    bool success = true;
    Hook *hooks = instr->hook;
    HooksRemoveResult res;
    while (hooks != nullptr && success) {
        success = this->run_hook(output, module, 0, *hooks, printSubMsg,
                                 runningState);
        Hooks_remove_completed_hook(res, instr->hook, hooks);
        instr->hook = res.newList;
        hooks = res.nextHook;
    }
    if (success) {
        opcode = instr->original_opcode;
    }
    return success;
}

HooksPrimitiveFunc *InstrumentationManager::start_primitive_call_interception(
    Module &m, uint32_t target_func) {
    if (this->has_HooksOnAroundFunction(target_func)) {
        return this->hooks_around_prim_funcs[target_func];
    }

    // The first time for which an instrumentation occurs for the primitive
    // func
    HooksPrimitiveFunc *instr = this->new_Primitive_Instrumentation();
    if (instr != nullptr) {
        instr->original_func = (Primitive)m.functions[target_func].func_ptr;
        this->hooks_around_prim_funcs[target_func] = instr;
        m.functions[target_func].func_ptr =
            (void (*)()) & Instrumentation_interceptPrimitiveCall;
    }
    return instr;
}

bool InstrumentationManager::stop_primitive_call_interception(
    Module &m, uint32_t target_func) {
    if (!this->has_HooksOnAroundFunction(target_func)) {
        return false;
    }

    HooksPrimitiveFunc *hooks_func = this->hooks_around_prim_funcs[target_func];
    m.functions[target_func].func_ptr =
        (void (*)()) & hooks_func->original_func;

    this->delete_Primitive_Instrumentation(hooks_func);
    this->hooks_around_prim_funcs.erase(target_func);
    return true;
}

HooksWasmAddr *InstrumentationManager::start_wasm_addr_intercept(
    Module &module, const uint32_t addr, HookMoment moment) {
    if (this->has_HookOnWasmAddr(addr, moment)) {
        if (moment == HookBefore) {
            return this->instr_wasm_addr_before[addr];
        } else {
            return this->instr_wasm_addr_after[addr];
        }
    }

    // The first time for which an instrumentation occurs for the wasm
    // address
    HooksWasmAddr *instr = this->new_WasmAddress_Instrumentation();
    if (instr != nullptr) {
        instr->address = addr;
        instr->original_opcode = module.bytes[addr];
        module.bytes[addr] = INSTRUMENTATION_INTERCEPT_OPCODE;
        instr->hook = nullptr;
        if (moment == HookBefore) {
            this->instr_wasm_addr_before[addr] = instr;
        } else {
            this->instr_wasm_addr_after[addr] = instr;
        }
    }
    return instr;
}

bool InstrumentationManager::runHookForOnEventHandling(const Channel &output,
                                                       Module *module) {
    Hook *hookToRun = this->hooksForOnEventHandling;
    if (hookToRun == nullptr || CallbackHandler::events->empty()) {
        if (hookToRun == nullptr) {
            printf(
                "TODO: hooksForOnEventHandling: there is no hook found to "
                "be "
                "run on newly pushed events. We will now therefore undo "
                "the instrumentation but we still need to decide what "
                "would be the default behaviour. Pause for instance?\n");
            this->stopRunningHooksOnEventsHandled();
        }
        return false;
    }

    Event event = CallbackHandler::events->front();
    uint32_t sizeBeforeHooks = CallbackHandler::events->size();
    bool eventRemoved = false;
    HooksRemoveResult res;
    while (hookToRun != nullptr) {
        eventRemoved = hookToRun->kind == EventRemove;
        this->run_hook_on_handled_event(output, *module, *hookToRun, &event);
        Hooks_remove_completed_hook(res, this->hooksForOnEventHandling,
                                    hookToRun);
        this->hooksForOnEventHandling = res.newList;

        if (eventRemoved) {
            // no need to run next hooks as event got removed
            break;
        }
        hookToRun = res.nextHook;
    }

    uint32_t sizeAfterHooks = CallbackHandler::events->size();
    if (eventRemoved && sizeAfterHooks >= sizeBeforeHooks) {
        printf(
            "TODO: the edge case occurred where new events got pushed into the "
            "queue (because of interrupts or hooks) while removing ones "
            "because of hooks. You need to introduce IDs for each event to "
            "know when to stop the isntrumentation.\n");
    }
    return eventRemoved && !CallbackHandler::events->empty();
}

void InstrumentationManager::runHooksForOnNewEvent(const Channel &output,
                                                   Module *module) {
    while (!CallbackHandler::pendingEvents->empty()) {
        Hook *hookToRun = this->hooksForOnNewEvent;
        if (hookToRun == nullptr) {
            printf(
                "TODO: runHooksforOnNewEvent: there is no hook found to be "
                "run on newly pushed events. We will now therefore undo "
                "the instrumentation but we still need to decide what "
                "would be the default behaviour. Pause for instance?\n");
            this->stopRunningHooksOnNewEvents();
            return;
        }

        Event *ev = CallbackHandler::pendingEvents->front();
        HooksRemoveResult res;
        while (hookToRun != nullptr) {
            this->run_hook_on_new_event(output, *module, *hookToRun, ev);
            Hooks_remove_completed_hook(res, this->hooksForOnNewEvent,
                                        hookToRun);
            this->hooksForOnNewEvent = res.newList;
            if (CallbackHandler::pendingEvents->empty() ||
                ev != CallbackHandler::pendingEvents->front()) {
                // event got removed by last hook
                // do no run remaining hooks
                break;
            }
            hookToRun = res.nextHook;
        }
        if (!CallbackHandler::pendingEvents->empty() &&
            ev == CallbackHandler::pendingEvents->front()) {
            // event did not get removed by any hook
            // add event to main queue
            CallbackHandler::push_event(ev);
            CallbackHandler::pendingEvents->pop_front();
        }
    }
}

/*
 * Methods that stop instrumentation
 */

void InstrumentationManager::stopRunningHooksOnNewEvents() {
    while (!CallbackHandler::pendingEvents->empty()) {
        // put pending events into main queue
        Event *ev = CallbackHandler::pendingEvents->front();
        CallbackHandler::pendingEvents->pop_front();
        CallbackHandler::push_event(ev);
    }
    CallbackHandler::pendingEventsActivated = false;

    Hooks_free_hooks(this->hooksForOnNewEvent);
}

void InstrumentationManager::stopRunningHooksOnEventsHandled() {
    this->interceptEvents = false;
    Hooks_free_hooks(this->hooksForOnEventHandling);
}

void InstrumentationManager::stopRunningHooksOnError() {
    this->interceptError = false;
    Hooks_free_hooks(this->hooksForOnError);
}

bool Instrumentation_interceptPrimitiveCall(Module *m) {
    return m->warduino->debugger->instrument.runHooksOnInterceptedFuncCall(
        *m->warduino->debugger->channel, m, &m->warduino->logicalClock,
        m->warduino->program_state);
}
