#pragma once
#include <cstdint>
#include <unordered_map>

#include "../Utils/sockets.h"
#include "../WARDuino/structs.h"
#include "./action.h"

typedef struct InstrumentationPrimitiveFunc {
    uint32_t func_idx;  // func for which the around action is registered
    Primitive original_func{};  // original function that is restored
    AroundAction *action{};     // action to perform instead of original_func
} InstrumentationPrimitiveFunc;

class InstrumentationManager {
   private:
    Channel *fun_call_channel{};

    std::unordered_map<uint32_t, InstrumentationPrimitiveFunc *>
        instr_primitive_funcs{};

    void free_Primitive_func_instr(InstrumentationPrimitiveFunc *instr);

    bool do_remote_call(Channel &channel, Module *m,
                        InstrumentationPrimitiveFunc *instr);

   public:
    InstrumentationManager();

    AroundFunction *new_AroundFunction();

    void free_AroundFunction(AroundFunction *around);

    bool add_AroundFunction(Module &m, AroundFunction *around);

    bool has_AroundFunction(uint32_t funID);

    bool apply_primitive_call_instrumentation(Channel &channel, Module *module);

    void registerAroundFunctionChannel(Channel *channel);
};

bool Instrumentation_interceptPrimitiveCall(Module *module);
