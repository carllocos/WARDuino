#pragma once
#include <cstdint>
#include <unordered_map>

#include "../Instrumentation/schedule.h"
#include "../Instrumentation/timestamp.h"
#include "../Utils/sockets.h"
#include "../WARDuino/structs.h"
#include "./action.h"

typedef struct InstrumentationPrimitiveFunc {
    uint32_t func_idx;  // func for which the around action is registered
    Primitive original_func{};  // original function that is restored
    Action *action{};           // action to perform instead of original_func
} InstrumentationPrimitiveFunc;

class InstrumentationManager {
   private:
    Channel *fun_call_channel{};

    std::unordered_map<uint32_t, InstrumentationPrimitiveFunc *>
        instr_primitive_funcs{};

    InstrumentationPrimitiveFunc *new_Primitive_Instrumentation();

    void remove_completed_action(InstrumentationPrimitiveFunc *inst,
                                 Action *action_completed);

    bool do_remote_call(Channel &channel, Module *m,
                        InstrumentationPrimitiveFunc *instr);

    bool do_value_substitution(Module *module, uint32_t func_called,
                               Action *action);

    InstrumentationPrimitiveFunc *start_primitive_call_interception(
        Module &m, uint32_t target_func);

   public:
    InstrumentationManager();

    bool addAroundFunctionAction(Module &m, uint32_t func_idx,
                                 const Action &around);

    bool has_AroundFunction(uint32_t funID);

    bool isAddActionAllowed(uint32_t funID);

    bool apply_primitive_call_instrumentation(Module *module,
                                              TimeStamp *currentTime);

    void registerAroundFunctionChannel(Channel *channel);
};

bool Instrumentation_interceptPrimitiveCall(Module *module);
