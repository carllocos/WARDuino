#pragma once
#include <cstdint>
#include <unordered_map>

#include "../Utils/sockets.h"
#include "../WARDuino/structs.h"
#include "./action.h"

typedef struct InstrumentationPrimitiveFunc {
    Primitive original_func{};
    AroundFunction *action{};
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
