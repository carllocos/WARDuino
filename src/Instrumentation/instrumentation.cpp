#include "instrumentation.h"

#include "../Interrupts/interrupt_remote_call.h"
#include "../Interrupts/interrupts.h"
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

bool InstrumentationManager::has_AroundFunction(uint32_t funID) {
    return this->instr_primitive_funcs.count(funID) > 0;
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
    AroundAction *action = this->instr_primitive_funcs[funID]->action;
    return action == nullptr || action->schedule.kind != ScheduleAlways;
}

bool InstrumentationManager::addAroundFunctionAction(
    Module &m, uint32_t func_idx, const AroundAction &action) {
    if (func_idx > m.function_count) {
        return false;
    } else if (func_idx < m.import_count) {
        InstrumentationPrimitiveFunc *instr =
            this->start_primitive_call_interception(m, func_idx);
        AroundAction *cpy{};
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

void InstrumentationManager::remove_completed_action(
    InstrumentationPrimitiveFunc *inst, AroundAction *action_completed) {
    if (action_completed->schedule.kind == ScheduleAlways) {
        // No delete needed, action is scheduled for always
        return;
    }

    AroundAction *actions = inst->action;
    AroundAction *prev = nullptr;
    while (actions != nullptr) {
        if (actions == action_completed) {
            if (prev == nullptr) {
                inst->action = actions->nextAction;
            } else {
                prev->nextAction = actions->nextAction;
            }
            break;
        }
        prev = actions;
        actions = actions->nextAction;
    }

    if (actions != nullptr) {
        Actions_free_action(action_completed);
    }
}

bool InstrumentationManager::do_remote_call(
    Channel &channel, Module *module, InstrumentationPrimitiveFunc *instr) {
    if (this->fun_call_channel == nullptr) {
        VM_Exception_write("No channel set to perform around function call\n");
        return false;
    }

    // get fun idx
    uint32_t func_to_call = instr->func_idx;

    // get args
    Type *func_type = module->functions[instr->action->value.target_fidx].type;
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
                                                   AroundAction *action) {
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
    Module *module, TimeStamp *currentTime) {
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
    AroundAction *actionToRun =
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

    bool around_successful = true;
    switch (actionToRun->kind) {
        case RemoteCall:
            around_successful =
                this->do_remote_call(*this->fun_call_channel, module, instr);
            break;
        case ValueSubstitution:
            around_successful = this->do_value_substitution(
                module, primitive_called, instr->action);
            break;
        default:
            VM_Exception_write("Unsupported around action\n");
            around_successful = false;
    }

    this->remove_completed_action(instr, actionToRun);

    if (!around_successful) {
        return false;
    }

    // After call instrumentation(s)
    return around_successful;
}

InstrumentationPrimitiveFunc *
InstrumentationManager::start_primitive_call_interception(
    Module &m, uint32_t target_func) {
    if (this->has_AroundFunction(target_func)) {
        return this->instr_primitive_funcs[target_func];
    }

    // The first time for which an instrumentation occurs for the primitive func
    InstrumentationPrimitiveFunc *instr = this->new_Primitive_Instrumentation();
    if (instr != nullptr) {
        instr->original_func = (Primitive)m.functions[target_func].func_ptr;
        this->instr_primitive_funcs[target_func] = instr;
        m.functions[target_func].func_ptr =
            (void (*)()) & Instrumentation_interceptPrimitiveCall;
    }
    return instr;
}

bool Instrumentation_interceptPrimitiveCall(Module *m) {
    return m->warduino->debugger->instrument
        .apply_primitive_call_instrumentation(m, &m->warduino->timeStamp);
}
