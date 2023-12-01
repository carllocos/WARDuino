#pragma once
#include "../../src/Structs/struct_stackvalue.h"
#include "vector"

class StackValueCompanion {
   private:
    std::vector<StackValue*> stacksToFree{};

   public:
    ~StackValueCompanion();

    StackValue* createStack(const uint32_t size);
    bool F32FromHexa(StackValue& value, const std::string hex);
    bool F64FromHexa(StackValue& value, const std::string hex);
};
