#pragma once

#include "../../src/WARDuino.h"
#include "callstackbuilder.h"
#include "dbgoutput.h"
#include "gtest/gtest.h"
#include "modulecompanion.h"
#include "stackvalue_companion.h"

class BaseFixture : public ::testing::Test {
   protected:
    uint8_t* wasm{};
    size_t wasm_len{};
    WARDuino* warduino;
    Module* wasm_module;
    DBGOutput* output{};
    CallstackBuilder* callstackBuilder{};
    ModuleCompanion* moduleCompanion{};
    StackValueCompanion* stackBuilder{};

    BaseFixture(uint8_t* t_wasm, size_t t_wasm_len);

    ~BaseFixture() override;

    void SetUp() override;
    void TearDown() override;
    std::string fullErrorMessage(const char* msg = nullptr);
};