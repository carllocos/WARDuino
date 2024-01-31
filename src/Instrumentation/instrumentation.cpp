#include "instrumentation.h"

#include "../Interrupts/interrupt_hook_on_addr.h"
#include "../Interrupts/interrupt_hook_on_event.h"
#include "../Interrupts/interrupt_remote_call.h"
#include "../Interrupts/interrupt_response.h"
#include "../Utils/macros.h"
#include "../WARDuino/vm_exception.h"

InstrumentationManager::InstrumentationManager() {}

void InstrumentationManager::registerAroundFunctionChannel(Channel *channel) {
    this->fun_call_channel = channel;
    this->lastObservedTime.nr_of_events = 0;
    this->lastObservedTime.nr_of_instructions =
        30;  // not set to 0 so the first instr can also be hooked upon
}

InstrumentationPrimitiveFunc *
InstrumentationManager::new_Primitive_Instrumentation() {
    return new InstrumentationPrimitiveFunc{};
}

InstrumentationWasmAddr *
InstrumentationManager::new_WasmAddress_Instrumentation() {
    return new InstrumentationWasmAddr{};
}

bool InstrumentationManager::has_AroundFunction(uint32_t funID) {
    return this->instr_primitive_funcs.count(funID) > 0;
}

bool InstrumentationManager::has_HookOnWasmAddr(uint32_t addr,
                                                InstrumentMoment moment) {
    switch (moment) {
        case InstrumentBefore:
            return this->instr_wasm_addr_before.count(addr) > 0;
        case InstrumentAfter:
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

bool InstrumentationManager::isAddHookAllowed(uint32_t funID) {
    if (!this->has_AroundFunction(funID)) return true;
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
    Hook *hook = this->instr_primitive_funcs[funID]->hook;
    return hook == nullptr || hook->schedule.kind != ScheduleAlways;
}

bool InstrumentationManager::addAroundFunctionHook(Module &m, uint32_t func_idx,
                                                   const Hook &around) {
    if (func_idx > m.function_count) {
        return false;
    } else if (func_idx < m.import_count) {
        InstrumentationPrimitiveFunc *instr =
            this->start_primitive_call_interception(m, func_idx);
        Hook *cpy{};
        if (instr == nullptr || (cpy = Hooks_copyHook(around)) == nullptr) {
            return false;
        }
        instr->hook = Hooks_add_and_sort(instr->hook, cpy);
        return true;
    } else {
        printf("TODO: addAroundFunctionHook for non primitive functions\n");
        return false;
    }
}

bool InstrumentationManager::removeHooksOnWasmAddress(
    Module &module, uint32_t addr, const InstrumentMoment moment) {
    if (!isToPhysicalAddrPossible(addr, &module)) {
        // address is not in module
        return false;
    }
    if (!this->has_HookOnWasmAddr(addr, moment)) {
        return false;
    }

    InstrumentationWasmAddr *instr;
    std::unordered_map<uint32_t, InstrumentationWasmAddr *> *mapForErase;
    bool restoreOpcode;
    if (moment == InstrumentBefore) {
        instr = instr_wasm_addr_before[addr];
        mapForErase = &this->instr_wasm_addr_before;
        restoreOpcode = !this->has_HookOnWasmAddr(addr, InstrumentAfter);
    } else {
        instr = instr_wasm_addr_after[addr];
        mapForErase = &this->instr_wasm_addr_after;
        restoreOpcode = !this->has_HookOnWasmAddr(addr, InstrumentBefore);
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

bool InstrumentationManager::addHookOnWasmAddress(
    Module &module, uint32_t addr, Hook &hook, const InstrumentMoment moment) {
    if (!isToPhysicalAddrPossible(addr, &module)) {
        // address is not in module
        return false;
    }
    InstrumentationWasmAddr *instr =
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
    hook.nextHook = nullptr;
    this->hooksForOnNewEvent = Hooks_add_and_sort(this->hooksForOnNewEvent, h);

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
        VM_Exception_write("No channel set to perform around function call\n");
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
        VM_Exception_write("Remotecall failed error_code%" PRIu8 " \n",
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
            VM_Exception_write("No Substitute value provided\n");
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
    uint8_t *pc_of_call = findStartOfLEB128(module->pc_ptr - 1);
    uint32_t primitive_called = read_LEB_32(&pc_of_call);

    // Before call instrumentation

    // Around call instrumentation(s)

    auto iterator = instr_primitive_funcs.find(primitive_called);
    if (iterator == instr_primitive_funcs.end() ||
        iterator->second->hook == nullptr) {
        VM_Exception_write(
            "No Instrumentation registered for primitive %" PRIu32 "\n",
            primitive_called);
        // TODO ADD subscription message for no instrumentation registered
        return false;
    }

    InstrumentationPrimitiveFunc *instr = iterator->second;
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
            VM_Exception_write("No hook scheduled for primitive call\n");
            return false;
        }
    }

    auto printSubMsg = [](std::function<void()> hookOutput) {};
    Event *noEvent = nullptr;
    bool around_successful =
        this->run_hook(output, *module, primitive_called, *hookToRun,
                       printSubMsg, runningState, noEvent);
    instr->hook = Hooks_remove_completed_hook(instr->hook, hookToRun);

    // After call instrumentation(s)
    return around_successful;
}

bool InstrumentationManager::run_hook(
    const Channel &output, Module &module, uint32_t local_fidx, Hook &hook,
    std::function<void(std::function<void()>)> sendSubscriptionMsg,
    RunningState &runningState, Event *ev) {
    switch (hook.kind) {
        case ProxyCall:
        case RemoteCall:
            return this->do_remote_call(*this->fun_call_channel, &module,
                                        local_fidx, hook.value.target_fidx,
                                        hook.kind == ProxyCall);
        case ValueSubstitution:
            return this->do_value_substitution(&module, local_fidx, &hook);
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
            sendSubscriptionMsg(printState);
            if (!success) {
                VM_Exception_write("Unexisting State to inspect\n");
                return false;
            }
            return true;
        }
        case ChangeRunningState:
            module.warduino->program_state = hook.value.runState;
            return true;
        case EventRemove:
            if (!CallbackHandler::pendingEvents->empty()) {
                CallbackHandler::pendingEvents->pop_front();
            }
            return true;
        case EventInspect:
            Interrupt_HookOnEvent_send_Binary_subscribe_message(output, *ev);
            return true;
        default:
            VM_Exception_write("Unsupported around hook\n");
            return false;
    }
}

void InstrumentationManager::run_hook_on_new_event(const Channel &output,
                                                   Module &module, Hook &hook,
                                                   Event *ev) {
    uint32_t noFunction = 0;
    auto printSubMsg = [&output](std::function<void()> hookOutput) {
        Interrupt_HookOnEvent_send_JSON_subscribe_message(
            output, HookOnNewEvent, hookOutput);
    };

    RunningState unUsedRunningState = WARDUINOpause;
    this->run_hook(output, module, noFunction, hook, printSubMsg,
                   unUsedRunningState, ev);
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
        if (!has_HookOnWasmAddr(addr, InstrumentAfter)) {
            continue;
        }
        InstrumentationWasmAddr *instr = this->instr_wasm_addr_after[addr];
        auto printSubMsg = [&output, addr](std::function<void()> hookOutput) {
            Interrupt_HookOnAddr_send_JSON_subscribe_message(
                output, InstrumentAfter, addr, hookOutput);
        };

        Hook *hooks = instr->hook;
        Event *noEvent = nullptr;
        while (hooks != nullptr) {
            this->run_hook(output, *module, 0, *hooks, printSubMsg,
                           runningState, noEvent);
            instr->hook = Hooks_remove_completed_hook(instr->hook, hooks);
            hooks = hooks->nextHook;
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
        auto instr = this->has_HookOnWasmAddr(addr, InstrumentBefore)
                         ? this->instr_wasm_addr_before[addr]
                         : this->instr_wasm_addr_after[addr];
        opcode = instr->original_opcode;
        module->pc_ptr += 1;
    } else {
        this->lastObservedTime = *currentTime;
        if (this->has_HookOnWasmAddr(addr, InstrumentBefore)) {
            success = this->do_before_wasm_addr_hooks(
                output, *module, *currentTime, addr, opcode, runningState);
            upcodeRestored = true;
        }

        if (runningState != WARDUINOpause) {
            module->pc_ptr += 1;
        }

        if (this->has_HookOnWasmAddr(addr, InstrumentAfter)) {
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
    if (!has_HookOnWasmAddr(addr, InstrumentBefore)) {
        VM_Exception_write(
            "No hook registered on instrumented addr %" PRIu32 "\n", addr);
        return false;
    }
    InstrumentationWasmAddr *instr = this->instr_wasm_addr_before[addr];

    auto printSubMsg = [&output, addr](std::function<void()> hookOutput) {
        Interrupt_HookOnAddr_send_JSON_subscribe_message(
            output, InstrumentBefore, addr, hookOutput);
    };
    bool success = true;
    Hook *hooks = instr->hook;
    Event *noEvent = nullptr;
    while (hooks != nullptr && success) {
        success = this->run_hook(output, module, 0, *hooks, printSubMsg,
                                 runningState, noEvent);
        instr->hook = Hooks_remove_completed_hook(instr->hook, hooks);
        if (!success) {
            break;
        }
        hooks = hooks->nextHook;
    }
    if (success) {
        opcode = instr->original_opcode;
    }
    return success;
}

InstrumentationPrimitiveFunc *
InstrumentationManager::start_primitive_call_interception(
    Module &m, uint32_t target_func) {
    if (this->has_AroundFunction(target_func)) {
        return this->instr_primitive_funcs[target_func];
    }

    // The first time for which an instrumentation occurs for the primitive
    // func
    InstrumentationPrimitiveFunc *instr = this->new_Primitive_Instrumentation();
    if (instr != nullptr) {
        instr->original_func = (Primitive)m.functions[target_func].func_ptr;
        this->instr_primitive_funcs[target_func] = instr;
        m.functions[target_func].func_ptr =
            (void (*)()) & Instrumentation_interceptPrimitiveCall;
    }
    return instr;
}

InstrumentationWasmAddr *InstrumentationManager::start_wasm_addr_intercept(
    Module &module, const uint32_t addr, InstrumentMoment moment) {
    if (this->has_HookOnWasmAddr(addr, moment)) {
        if (moment == InstrumentBefore) {
            return this->instr_wasm_addr_before[addr];
        } else {
            return this->instr_wasm_addr_after[addr];
        }
    }

    // The first time for which an instrumentation occurs for the wasm
    // address
    InstrumentationWasmAddr *instr = this->new_WasmAddress_Instrumentation();
    if (instr != nullptr) {
        instr->address = addr;
        instr->original_opcode = module.bytes[addr];
        module.bytes[addr] = INSTRUMENTATION_INTERCEPT_OPCODE;
        instr->hook = nullptr;
        if (moment == InstrumentBefore) {
            this->instr_wasm_addr_before[addr] = instr;
        } else {
            this->instr_wasm_addr_after[addr] = instr;
        }
    }
    return instr;
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
        while (hookToRun != nullptr) {
            this->run_hook_on_new_event(output, *module, *hookToRun, ev);
            this->hooksForOnNewEvent = Hooks_remove_completed_hook(
                this->hooksForOnNewEvent, hookToRun);
            if (CallbackHandler::pendingEvents->empty() ||
                ev != CallbackHandler::pendingEvents->front()) {
                // event got removed by last hook
                // do no run remaining hooks
                break;
            }
            hookToRun = hookToRun->nextHook;
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

    // TODO refactor remove hooks
    while (this->hooksForOnNewEvent != nullptr) {
        Hook *hookToFree = this->hooksForOnNewEvent;
        this->hooksForOnNewEvent = this->hooksForOnNewEvent->nextHook;
        Hooks_free_hook(hookToFree);
    }
}

bool Instrumentation_interceptPrimitiveCall(Module *m) {
    return m->warduino->debugger->instrument.runHooksOnInterceptedFuncCall(
        *m->warduino->debugger->channel, m, &m->warduino->logicalClock,
        m->warduino->program_state);
}
