#include "stackvalue_companion.h"

#include "../../src/WARDuino.h"
#include "stdio.h"

StackValueCompanion::~StackValueCompanion() {
    for (auto stack : this->stacksToFree) {
        free(stack);
    }
}
StackValue* StackValueCompanion::createStack(uint32_t size) {
    StackValue* stack = (StackValue*)malloc(sizeof(StackValue) * size);
    this->stacksToFree.push_back(stack);
    return stack;
}

bool StackValueCompanion::F32FromHexa(StackValue& value,
                                      const std::string hex) {
    uint32_t val{};
    if (sscanf(hex.c_str(), "%x", &val) != 1) {
        return false;
    }

    float floatValue{};
    memcpy(&floatValue, &val, sizeof(float));
    value.value.f32 = floatValue;
    value.value_type = F32;
    return true;
}

bool StackValueCompanion::F64FromHexa(StackValue& value,
                                      const std::string hex) {
    double val;
    if (sscanf(hex.c_str(), "%la", &val) != 1) {
        return false;
    }
    value.value.f64 = val;
    value.value_type = F64;
    return true;
}
