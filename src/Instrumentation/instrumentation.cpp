#include "instrumentation.h"

#include "../Interrupts/interrupt_remote_call.h"
#include "../Interrupts/interrupt_response.h"
#include "../Utils/macros.h"
#include "../WARDuino/vm_exception.h"

InstrumentationManager::InstrumentationManager() {}

void InstrumentationManager::registerAroundFunctionChannel(Channel *channel) {
    this->fun_call_channel = channel;
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

bool InstrumentationManager::has_ActionOnWasmAddr(uint32_t addr,
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

bool InstrumentationManager::isAddActionAllowed(uint32_t funID) {
    if (!this->has_AroundFunction(funID)) return true;
    // Dissallows actions that have been scheduled for always if one is
    // alread in place

    // TODO: decide whether even if you have always adding this is allowed
    // because it just replaces it or becomes another action. if the action kind
    // is always tou replace the old one with the new one if the action is
    // something else than always such as once then that action is put in front
    // of the always so that once the once is consumed the always remains.
    Action *action = this->instr_primitive_funcs[funID]->action;
    return action == nullptr || action->schedule.kind != ScheduleAlways;
}

bool InstrumentationManager::addAroundFunctionAction(Module &m,
                                                     uint32_t func_idx,
                                                     const Action &action) {
    if (func_idx > m.function_count) {
        return false;
    } else if (func_idx < m.import_count) {
        InstrumentationPrimitiveFunc *instr =
            this->start_primitive_call_interception(m, func_idx);
        Action *cpy{};
        if (instr == nullptr || (cpy = Actions_copyAction(action)) == nullptr) {
            return false;
        }
        instr->action = Actions_add_and_sort(instr->action, cpy);
        return true;
    } else {
        printf("TODO: addAroundFunctionAction for non primitive functions\n");
        return false;
    }
}

bool InstrumentationManager::addActionOnWasmAddress(
    Module &module, uint32_t addr, Action &action,
    const InstrumentMoment moment) {
    if (!isToPhysicalAddrPossible(addr, &module)) {
        // address is not in module
        return false;
    }
    InstrumentationWasmAddr *instr =
        this->start_wasm_addr_intercept(module, addr, moment);

    Action *cpy{};
    if (instr == nullptr || (cpy = Actions_copyAction(action)) == nullptr) {
        return false;
    }
    instr->action = Actions_add_and_sort(instr->action, cpy);
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
                                            uint32_t func_to_call) {
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
                              channel, &response);
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
                                                   Action *action) {
    Type *type = module->functions[func_called].type;
    module->sp -= type->param_count;  // pop args
    if (type->result_count > 0) {
        if (action->value.result == nullptr) {
            VM_Exception_write("No Substitute value provided\n");
            return false;
        }
        module->sp += 1;
        StackValue *value = &module->stack[module->sp];
        value->value_type = action->value.result->value_type;
        value->value = action->value.result->value;
    }
    return true;
}

bool InstrumentationManager::apply_primitive_call_instrumentation(
    const Channel &output, Module *module, TimeStamp *currentTime) {
    uint8_t *pc_of_call = findStartOfLEB128(module->pc_ptr - 1);
    uint32_t primitive_called = read_LEB_32(&pc_of_call);

    // Before call instrumentation

    // Around call instrumentation(s)

    auto iterator = instr_primitive_funcs.find(primitive_called);
    if (iterator == instr_primitive_funcs.end() ||
        iterator->second->action == nullptr) {
        VM_Exception_write(
            "No Instrumentation registered for primitive %" PRIu32 "\n",
            primitive_called);
        // TODO ADD subscription message for no instrumentation registered
        return false;
    }

    InstrumentationPrimitiveFunc *instr = iterator->second;
    Action *actionToRun =
        Actions_nextScheduledAction(instr->action, *currentTime);
    if (actionToRun == nullptr) {
        // We did not find an action to execute now, but maybe we have to wait
        // for an event to occur
        if (Actions_isActionWaitingForEvent(instr->action, *currentTime)) {
            printf(
                "TODO: restore the PC to the position before the call "
                "(and pause the VM)?");
            WARDuino::instance()->program_state = WARDUINOpause;
            // TODO send messages via subscription for waiting for incoming
            // event to occur
            return true;
        } else {
            VM_Exception_write("No action scheduled for primitive call\n");
            return false;
        }
    }

    bool around_successful =
        this->run_action(output, *module, primitive_called, *actionToRun);
    instr->action = Actions_remove_completed_action(instr->action, actionToRun);

    // After call instrumentation(s)
    return around_successful;
}

bool InstrumentationManager::run_action(const Channel &output, Module &module,
                                        uint32_t local_fidx, Action &action) {
    switch (action.kind) {
        case RemoteCall:
            return this->do_remote_call(*this->fun_call_channel, &module,
                                        local_fidx, action.value.target_fidx);
        case ValueSubstitution:
            return this->do_value_substitution(&module, local_fidx, &action);
        case StateInspect: {
            const Module *m = &module;
            if (!Interrupt_Inspect_inspect_json_output(output, m,
                                                       *action.value.state)) {
                VM_Exception_write("Unexisting State to inspect\n");
                return false;
            }
            return true;
        }
        default:
            VM_Exception_write("Unsupported around action\n");
            return false;
    }
}

bool InstrumentationManager::apply_wasm_addr_instrumentation(
    const Channel &output, Module *module, TimeStamp *currentTime,
    uint8_t &opcode) {
    uint32_t addr = toVirtualAddress(module->pc_ptr - 1, module);
    return this->do_before_wasm_addr_actions(output, *module, *currentTime,
                                             addr, opcode);
}

bool InstrumentationManager::do_before_wasm_addr_actions(const Channel &output,
                                                         Module &module,
                                                         TimeStamp &currentTime,
                                                         uint32_t addr,
                                                         uint8_t &opcode) {
    if (!has_ActionOnWasmAddr(addr, InstrumentBefore)) {
        VM_Exception_write(
            "No action registered on instrumented addr %" PRIu32 "\n", addr);
        return false;
    }
    InstrumentationWasmAddr *instr = this->instr_wasm_addr_before[addr];
    bool success = this->run_action(output, module, 0, *instr->action);
    instr->action =
        Actions_remove_completed_action(instr->action, instr->action);
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
    if (this->has_ActionOnWasmAddr(addr, moment)) {
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
        instr->action = nullptr;
        if (moment == InstrumentBefore) {
            this->instr_wasm_addr_before[addr] = instr;
        } else {
            this->instr_wasm_addr_after[addr] = instr;
        }
    }
    return instr;
}

bool Instrumentation_interceptPrimitiveCall(Module *m) {
    return m->warduino->debugger->instrument
        .apply_primitive_call_instrumentation(*m->warduino->debugger->channel,
                                              m, &m->warduino->timeStamp);
}
