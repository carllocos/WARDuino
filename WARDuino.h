#ifndef WAC_H
#define WAC_H
#include "structs.h"
#include "recording.h"

#include <map>
#include <queue>  // std::queue
#include <set>

class WARDuino;  // predeclare for it work in the module decl

typedef struct Module {
    WARDuino *warduino;
    char *path;       // file path of the wasm module
    Options options;  // Config options

    uint32_t byte_count;  // number of bytes in the module
    uint8_t *bytes;       // module content/bytes

    uint32_t type_count;  // number of function types
    Type *types;          // function types

    uint32_t import_count;    // number of leading imports in functions
    uint32_t function_count;  // number of function (including imports)
    Block *functions;         // imported and locally defined functions
    std::map<uint8_t *, Block *>
        block_lookup;  // map of module byte position to Blocks
    // same length as byte_count
    uint32_t start_function;  // function to run on module load
    Table table;
    Memory memory;
    uint32_t global_count;  // number of globals
    StackValue *globals;    // globals
    // Runtime state
    uint8_t *pc_ptr;                   // program counter
    uint8_t *pc_error = nullptr;
    int sp;                            // operand stack pointer
    int fp;                            // current frame pointer into stack
    StackValue stack[STACK_SIZE];      // main operand stack
    int csp;                           // callstack pointer
    Frame callstack[CALLSTACK_SIZE];   // callstack
    uint32_t br_table[BR_TABLE_SIZE];  // br_table branch indexes

    char *exception;  // exception is set when the program fails

    std::vector<Record*> snapshots;
    uint32_t snapshot_count;
} Module;

typedef bool (*Primitive)(Module *);

typedef struct PrimitiveEntry {
    const char *name;
    Primitive f;
    Type t;
} PrimitiveEntry;

enum RunningState {
    WARDUINOrun,
    WARDUINOpause,
    WARDUINOstep,
    WARDuinorestart,
    WARDuinoProxyRun
};

typedef struct {
    Module *m;
    Options options;
    uint8_t *new_bytes;
    uint32_t byte_count;
    RunningState state;
    /* uint8_t* pc_error; */
} RmvModule;

typedef struct {
    std::set<uint32_t> proxies = {};
    char *host = nullptr;
    int port = 0;
    bool connected = false;
} ProxyClient;


class WARDuino {
   private:
    std::vector<Module *> modules = {};
    std::deque<uint8_t *> parsedInterrups = {};

    // factualy volatile

    volatile bool interruptWrite;
    volatile bool interruptRead;
    bool interruptEven = true;
    uint8_t interruptLastChar;
    std::vector<uint8_t> interruptBuffer;
    long interruptSize;

   public:
    // vector, we expect few breakpoints
    std::set<uint8_t *> breakpoints = {};

    // Breakpoint to skip in the next interpretation step
    uint8_t *skipBreakpoint = nullptr;
    uint8_t *until_pc = nullptr;
    int until_csp = -1;

    // needed to potentialy start warduino in a paused state
    RunningState initial_runstate = WARDUINOrun;

    WARDuino();

    int run_module(RmvModule *m);

    Module *load_module(uint8_t *bytes, uint32_t byte_count, Options options);

    void unload_module(Module *m);

    bool invoke(RmvModule *m, uint32_t fidx);

    uint32_t get_export_fidx(Module *m, const char *name);

    void handleInterrupt(size_t len, uint8_t *buff);

    // breakpoints
    void addBreakpoint(uint8_t *loc);

    void delBreakpoint(uint8_t *loc);

    bool isBreakpoint(uint8_t *loc);

    RmvModule *removable(Module *m);
    // Get interrupt or NULL if none
    uint8_t *getInterrupt();
};

#endif
