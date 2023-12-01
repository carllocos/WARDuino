#include "base_interruptfixture.h"

#include "../../../src/Utils/util.h"

BaseFixture::BaseFixture(uint8_t* t_wasm, size_t t_wasm_len)
    : wasm(t_wasm), wasm_len(t_wasm_len), warduino(WARDuino::instance()) {}

BaseFixture::~BaseFixture() {}

void BaseFixture::SetUp() {
    Options opts = {.disable_memory_bounds = false,
                    .mangle_table_index = false,
                    .dlsym_trim_underscore = false,
                    .return_exception = true};
    this->wasm_module = warduino->load_module(wasm, wasm_len, opts);
    this->output = new DBGOutput();
    if (!this->output->open()) {
        FAIL() << "could not open connect tmpfile";
    }

    callstackBuilder = new CallstackBuilder(this->wasm_module);
    moduleCompanion = new ModuleCompanion(this->wasm_module);
    stackBuilder = new StackValueCompanion{};
}

std::string BaseFixture::fullErrorMessage(const char* msg) {
    std::string errorMsg{msg == nullptr ? "" : msg};
    errorMsg += "\nReceived following lines:\n";
    this->output->appendReadLines(&errorMsg);
    return errorMsg;
}

void BaseFixture::TearDown() {
    warduino->unload_module(wasm_module);
    wasm_module = nullptr;
    delete callstackBuilder;
    delete moduleCompanion;
    delete stackBuilder;
}