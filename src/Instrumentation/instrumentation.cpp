#include "instrumentation.h"

#include "../Interrupts/interrupt_remote_call.h"
#include "../Interrupts/interrupts.h"
#include "../Utils/macros.h"
#include "../Utils/util.h"
#include "../WARDuino/vm_exception.h"

InstrumentationManager::InstrumentationManager() {}

void InstrumentationManager::registerAroundFunctionChannel(Channel *channel) {
    this->fun_call_channel = channel;
}

AroundFunction *InstrumentationManager::new_AroundFunction() {
    return new AroundFunction{};
}

void InstrumentationManager::free_AroundFunction(AroundFunction *around) {
    delete around;
}

bool InstrumentationManager::has_AroundFunction(uint32_t funID) {
    return this->instr_primitive_funcs.count(funID) > 0;
}

bool InstrumentationManager::add_AroundFunction(Module &m,
                                                AroundFunction *around) {
    if (around == nullptr || around->func_idx > m.function_count) {
        return false;
    } else if (around->func_idx < m.import_count) {
        auto instr = new InstrumentationPrimitiveFunc{};
        if (instr == nullptr) {
            return false;
        }

        instr->original_func =
            (Primitive)m.functions[around->func_idx].func_ptr;
        instr->action = around;
        this->instr_primitive_funcs[around->func_idx] = instr;
        m.functions[around->func_idx].func_ptr =
            (void (*)()) & Instrumentation_interceptPrimitiveCall;
        return true;
    } else {
        printf("TODO: add_AroundFunction for non primitive functions\n");
        return false;
    }
}

void InstrumentationManager::free_Primitive_func_instr(
    InstrumentationPrimitiveFunc *instr) {
    delete instr;
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

bool InstrumentationManager::do_remote_call(
    Channel &channel, Module *module, InstrumentationPrimitiveFunc *instr) {
    if (this->fun_call_channel == nullptr) {
        VM_Exception_write("No channel set to perform around function call\n");
        return false;
    }

    // get fun idx
    uint32_t func_to_call = instr->action->action.target_fidx;

    // get args
    Type *func_type = module->functions[instr->action->func_idx].type;
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

bool InstrumentationManager::apply_primitive_call_instrumentation(
    Channel &channel, Module *module) {
    uint8_t *pc_of_call = findStartOfLEB128(module->pc_ptr - 1);
    uint32_t primitive_called = read_LEB_32(&pc_of_call);
    auto iterator = instr_primitive_funcs.find(primitive_called);
    if (iterator == instr_primitive_funcs.end()) {
        VM_Exception_write(
            "No Instrumentation registered for primitive %" PRIu32 "\n",
            primitive_called);
        // TODO ADD subscription message for no instrumentation registered
        return false;
    }

    InstrumentationPrimitiveFunc *instr = iterator->second;

    // before

    // around
    switch (instr->action->kind) {
        case RemoteCall:
            return this->do_remote_call(*this->fun_call_channel, module, instr);
        default:
            FATAL("Other kind not yet supported\n");
    }
    // after
}

bool Instrumentation_interceptPrimitiveCall(Module *m) {
    return m->warduino->debugger->instrument
        .apply_primitive_call_instrumentation(*m->warduino->debugger->channel,
                                              m);
}
