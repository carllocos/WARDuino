#pragma once

#include <array>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "Debug/debugger.h"
#include "Edward/proxy_supervisor.h"
#include "Structs/struct_module.h"
#include "WARDuino/CallbackHandler.h"
#include "WARDuino/WARDuino_constants.h"

extern char exception[512];

class WARDuino {
   private:
    static WARDuino *singleton;
    std::vector<Module *> modules = {};

    WARDuino();

    uint32_t get_main_fidx(Module *m);

   public:
    Debugger *debugger;
    RunningState program_state = WARDUINOrun;

    static WARDuino *instance();

    int run_module(Module *m);

    Module *load_module(uint8_t *bytes, uint32_t byte_count, Options options);

    void unload_module(Module *m);

    void update_module(Module *old_module, uint8_t *wasm, uint32_t wasm_len);

    bool invoke(Module *m, uint32_t fidx, uint32_t arity = 0,
                StackValue *args = nullptr);

    uint32_t get_export_fidx(Module *m, const char *name);

    void handleInterrupt(size_t len, uint8_t *buff) const;

    void instantiate_module(Module *m, uint8_t *bytes, uint32_t byte_count);

    void free_module_state(Module *m);
};
