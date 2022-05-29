#include "interrupt_operations.h"

#include <inttypes.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>

#include "WARDuino.h"
#include "debug.h"
#include "interrupt_protocol.h"
#include "mem.h"
#include "string.h"
#include "util.h"
#include "proxy.h"
#include "rfc.h"

#if SOCKET
#include "socket_server.h"
#endif

#include "printing.h"

/**
 * Validate if there are interrupts and execute them
 *
 * The various kinds of interrups are preceded by an identifier:
 *
 * - `0x01` : Continue running
 * - `0x02` : Halt the execution
 * - `0x03` : Pause execution
 * - `0x04` : Execute one operaion and then pause
 * - `0x06` : Add a breakpoint, the adress is specified as a pointer.
 *            The pointer should be specified as: 06[length][pointer]
 *            eg: 06 06 55a5994fa3d6
 * - `0x07` : Remove the breakpoint at the adress specified as a pointer if it
 *            exists (see `0x06`)
 * - `0x10` : Dump information about the program
 * - `0x11` :                  show locals
 * - `0x20` : Replace the content body of a function by a new function given
 *            as payload (immediately following `0x10`), see #readChange
 */

enum InteruptTypes {
    interruptRUN = 0x01,
    interruptHALT = 0x02,
    interruptPAUSE = 0x03,
    interruptSTEP = 0x04,
    interruptUntil = 0x05,
    interruptBPAdd = 0x06,
    interruptBPRem = 0x07,
    interruptState = 0x10,
    interruptUPDATEFun = 0x20,
    interruptUPDATELocal = 0x21,
    interruptRecvState = 0x22,
    interruptOffset = 0x23,
    interruptUPDATEMOD = 0x24,
    interruptMonitorProxies = 0x25,
    interruptProxyCall = 0x26,
    interruptToggleWiFi = 0x27,
    interruptRFCNoCache = 0x28,
    interruptRFCUseCache = 0x29,
};

enum ReceiveState {
    pcState = 0x01,
    breakpointsState = 0x02,
    callstackState = 0x03,
    globalsState = 0x04,
    tblState = 0x05,
    memState = 0x06,
    brtblState = 0x07,
    stackvalsState = 0x08,
    pcErrorState = 0x09
};

bool receivingData = false;

void doDumpLocals(Module *m);

void freeState(Module *m, uint8_t *interruptData);

uintptr_t readPointer(uint8_t **data);

bool saveState(Module *m, uint8_t *interruptData);

void registerRFCs(Module * m, uint8_t **data);

void registerHost(uint8_t **data);
StackValue *readRFCArgs(Block * func, uint8_t * data);
void setCacheRFCs(Module * m, uint8_t **data, bool cache);

// TODO inefficient. Keep extra state at each pushblock to ease opcode
// retrieval?
uint8_t *find_opcode(Module *m, Block *block) {
    auto find =
        std::find_if(std::begin(m->block_lookup), std::end(m->block_lookup),
                     [&](const std::pair<uint8_t *, Block *> &pair) {
                         return pair.second == block;
                     });
    uint8_t *opcode = nullptr;
    if (find != std::end(m->block_lookup)) {
        opcode = find->first;
    } else {
        // FIXME FATAL?
        debug("find_opcode: not found\n");
        exit(33);
    }
    return opcode;
}

void format_constant_value(char *buf, StackValue *v) {
    switch (v->value_type) {
        case I32:
            snprintf(buf, 255, R"("type":"i32","value":%)" PRIi32,
                     v->value.uint32);
            break;
        case I64:
            snprintf(buf, 255, R"("type":"i64","value":%)" PRIi64,
                     v->value.uint64);
            break;
        case F32:
            snprintf(buf, 255, R"("type":"f32","value":%.7f)", v->value.f32);
            break;
        case F64:
            snprintf(buf, 255, R"("type":"f64","value":%.7f)", v->value.f64);
            break;
        default:
            snprintf(buf, 255, R"("type":"%02x","value":"%)" PRIx64 "\"",
                     v->value_type, v->value.uint64);
    }
}

void doDump(RmvModule *rm) {
    Module *m = rm->m;
    // FIXME replace write

    debug("asked for doDump\n");
    wa_flush();
    wa_printf("DUMP!\n");
    wa_printf("{");

    // printf("asked for pc\n");
    // current PC
    wa_printf(R"("pc":"%p",)", (void *)m->pc_ptr);
    if(rm->m->pc_error != nullptr){
        wa_printf("\"pc_error\":\"%p\"," , rm->m->pc_error);
    }

    // start of bytes
    wa_printf(R"("start":["%p"],)", (void *)m->bytes);
    // printf("asked for bps\n");

    wa_printf("\"breakpoints\":[");
    size_t i = 0;
    for (auto bp : m->warduino->breakpoints) {
        wa_printf(R"("%p"%s)", bp,
                  (++i < m->warduino->breakpoints.size()) ? "," : "");
    }
    wa_printf("],");

    // printf("asked for stack\n");
    //stack
    wa_printf("\"stack\":[");
    char _value_str[256];
    for (int i = 0; i <= m->sp; i++) {
        auto v = &m->stack[i];
        format_constant_value(_value_str, v);
        wa_printf(R"({"idx":%d, %s}%s)", i, _value_str,
                  (i == m->sp) ? "" : ",");
    }
    wa_printf("],");

    // Callstack
    wa_printf("\"callstack\":[");
    for (int i = 0; i <= m->csp; i++) {
        Frame *f = &m->callstack[i];
        uint8_t *block_key =
            f->block->block_type == 0 ? nullptr : find_opcode(m, f->block);
        wa_printf(
            R"({"type":%u,"fidx":"0x%x","sp":%d,"fp":%d,"block_key":"%p", "ra":"%p"}%s)",
            f->block->block_type, f->block->fidx, f->sp, f->fp, block_key,
            static_cast<void *>(f->ra_ptr), (i < m->csp) ? "," : "");
    }

    // printf("asked for globals\n");
    // GLobals
    wa_printf("],\"globals\":[");
    for (uint32_t i = 0; i < m->global_count; i++) {
        char _value_str[256];
        auto v = m->globals + i;
        format_constant_value(_value_str, v);
        wa_printf(R"({"idx":%d,%s}%s)", i, _value_str,
                  ((i + 1) < m->global_count) ? "," : "");
    }
    wa_printf("]");  // closing globals

    // printf("asked for table\n");
    wa_printf(",\"table\":{\"max\":%d, \"init\":%d, \"elements\":[",
              m->table.maximum, m->table.initial);
    wa_flush();
    wa_write(m->table.entries, sizeof(uint32_t) * m->table.size);
    wa_printf("]}");  // closing table

    // printf("asked for mem\n");
    // memory
    uint32_t total_elems =
        m->memory.pages * (uint32_t)PAGE_SIZE;  // TODO debug PAGE_SIZE
    wa_printf(",\"memory\":{\"pages\":%d,\"max\":%d,\"init\":%d,\"bytes\":[",
              m->memory.pages, m->memory.maximum, m->memory.initial);

    wa_flush();
    wa_write(m->memory.bytes, total_elems * sizeof(uint8_t));
    wa_printf("]}");  // closing memory


    // printf("asked for br_table\n");
    wa_printf(",\"br_table\":{\"size\":\"0x%x\",\"labels\":[", BR_TABLE_SIZE);
    wa_flush();
    wa_write(m->br_table, BR_TABLE_SIZE * sizeof(uint32_t));
    wa_printf("]}}\n");
    wa_flush();
}

uint32_t read_L32(uint8_t **bytes) {
    // uint8_t *b = *bytes;
    uint32_t n = 0;
    memcpy(&n, *bytes, sizeof(uint32_t));
    *bytes += 4;
    return n;
}  // TODO replace with read_LEB_32? If keep Big endian use memcpy?

uint32_t read_B32(uint8_t **bytes) {
    uint8_t *b = *bytes;
    uint32_t n = (b[0] << 24) + (b[1] << 16) + (b[2] << 8) + b[3];
    *bytes += 4;
    return n;
}  // TODO replace with read_LEB_32? If keep Big endian use memcpy?

int read_B32_signed(uint8_t **bytes) {
    uint8_t *b = *bytes;
    int n = (b[0] << 24) + (b[1] << 16) + (b[2] << 8) + b[3];
    *bytes += 4;
    return n;
}  // TODO replace with read_LEB_32? If keep Big endian use memcpy?

uint16_t read_B16(uint8_t **bytes) {
    uint8_t *b = *bytes;
    uint32_t n = (b[0] << 8) + b[1];
    *bytes += 2;
    return n;
}

void freeState(Module *m, uint8_t *interruptData) {
    debug("freeing the program state\n");
    printf("freeing the program state\n");
    uint8_t *first_msg = nullptr;
    uint8_t *endfm = nullptr;
    first_msg = interruptData + 1;  // skip interruptRecvState
    endfm = first_msg + read_B32(&first_msg);

    // nullify state
    m->warduino->breakpoints.clear();
    m->csp = -1;
    m->sp = -1;
    memset(m->br_table, 0, BR_TABLE_SIZE);

    while (first_msg < endfm) {
        switch (*first_msg++) {
            case globalsState: {
                debug("receiving globals info\n");
                printf("receiving globals info\n");
                uint32_t amount = read_B32(&first_msg);
                debug("total globals %d\n", amount);
                // TODO if global_count != amount Otherwise set all to zero
                if (m->global_count != amount) {
                    debug("globals freeing state and then allocating\n");
                    if (m->global_count > 0) free(m->globals);
                    if (amount > 0)
                        m->globals = (StackValue *)acalloc(
                            amount, sizeof(StackValue), "globals");
                } else {
                    debug("globals setting existing state to zero\n");
                    for (uint32_t i = 0; i < m->global_count; i++) {
                        debug("decreasing global_count\n");
                        StackValue *sv = &m->globals[i];
                        sv->value_type = 0;
                        sv->value.uint32 = 0;
                    }
                }
                m->global_count = 0;
                break;
            }
            case tblState: {
                debug("receiving table info\n");
                printf("receiving table info\n");
                m->table.initial = read_B32(&first_msg);
                m->table.maximum = read_B32(&first_msg);
                uint32_t size = read_B32(&first_msg);
                debug("init %d max %d size %d\n", m->table.initial,
                      m->table.maximum, size);
                if (m->table.size != size) {
                    debug("old table size %d\n", m->table.size);
                    if (m->table.size != 0) free(m->table.entries);
                    m->table.entries = (uint32_t *)acalloc(
                        size, sizeof(uint32_t), "Module->table.entries");
                }
                m->table.size = 0;  // allows to accumulatively add entries
                break;
            }
            case memState: {
                debug("receiving memory info\n");
                printf("receiving memory info\n");
                // FIXME: init & max not needed
                m->memory.maximum = read_B32(&first_msg);
                m->memory.initial = read_B32(&first_msg);
                uint32_t pages = read_B32(&first_msg);
                debug("max %d init %d current page %d\n", m->memory.maximum,
                      m->memory.initial, pages);
                printf("max %d init %d current page %d\n", m->memory.maximum,
                      m->memory.initial, pages);
                // if(pages !=m->memory.pages){
                // if(m->memory.pages !=0)
                if(m->memory.bytes != nullptr){
                    free(m->memory.bytes);
                }
                m->memory.bytes =
                    (uint8_t *)acalloc(pages * PAGE_SIZE, sizeof(uint32_t),
                                       "Module->memory.bytes");
                m->memory.pages = pages;
                // }
                // else{
                //   //TODO fill memory.bytes with zeros
                // memset(m->memory.bytes, 0, m->memory.pages * PAGE_SIZE) ;
                // }
                break;
            }
            default: {
                debug("freeState: receiving unknown command\n");
                exit(33);  // FIXME replace
                break;
            }
        }
    }
    debug("done with first msg\n");
    /* printf("done with first msg\n"); */
}

uintptr_t readPointer(uint8_t **data) {
    uint8_t len = (*data)[0];
    uintptr_t bp = 0x0;
    for (size_t i = 0; i < len; i++) {
        bp <<= sizeof(uint8_t) * 8;
        bp |= (*data)[i + 1];
    }
    *data += 1 + len;  // skip pointer
    return bp;
}

bool saveState(Module *m, uint8_t *interruptData) {
    uint8_t *program_state = nullptr;
    uint8_t *endstate = nullptr;
    program_state = interruptData + 1;  // skip interruptRecvState
    endstate = program_state + read_B32(&program_state);

    while (program_state < endstate) {
        switch (*program_state++) {
            case pcState: {  // PC
                printf("reciving pc\n");
                m->pc_ptr = (uint8_t *)readPointer(&program_state);
                break;
            }
            case pcErrorState: {  // PC
                printf("reciving pc_error\n");
                m->pc_error = (uint8_t *)readPointer(&program_state);
                printf("pointer received %p\n", static_cast<void *>(m->pc_error));
                break;
            }
            case breakpointsState: {  // breakpoints
                uint8_t quantity_bps = *program_state++;
                printf("receiving breakpoints %" PRIu8 "\n", quantity_bps);
                for (size_t i = 0; i < quantity_bps; i++) {
                    auto bp = (uint8_t *)readPointer(&program_state);
                    m->warduino->addBreakpoint(bp);
                }
                break;
            }
            case callstackState: {
                debug("receiving callstack\n");
                printf("receiving callstack\n");
                uint16_t quantity = read_B16(&program_state);
                debug("quantity frames %" PRIu16 "\n", quantity);
                for (size_t i = 0; i < quantity; i++) {
                    uint8_t block_type = *program_state++;
                    m->csp += 1;
                    Frame *f = m->callstack + m->csp;
                    f->sp = read_B32_signed(&program_state);
                    f->fp = read_B32_signed(&program_state);
                    f->ra_ptr = (uint8_t *)readPointer(&program_state);
                    if (block_type == 0) {  // a function
                        debug("function block\n");
                        uint32_t fidx = read_B32(&program_state);
                        debug("function block idx=%" PRIu32 "\n", fidx);
                        f->block = m->functions + fidx;

                        if (f->block->fidx != fidx) {
                            printf("incorrect fidx: exp %" PRIu32 " got %" PRIu32
                                  ". Exiting program\n",
                                  fidx, f->block->fidx);
                            exit(55);
                        }
                        m->fp = f->sp + 1;
                    } else {
                        printf("non function block\n");
                        uint8_t *block_key =
                            (uint8_t *)readPointer(&program_state);
                        /* printf("block_key=%p\n", static_cast<void *>(block_key)); */
                        f->block = m->block_lookup[block_key];
                        if(f->block == nullptr){
                          printf("block_lookup cannot be nullptr\n");
                          exit(33);
                        }
                    }
                }
                break;
            }
            case globalsState: {  // TODO merge globalsState stackvalsState into
                                  // one case
                debug("receiving global state\n");
                printf("receiving globals\n");
                uint32_t quantity_globals = read_B32(&program_state);
                uint8_t valtypes[] = {I32, I64, F32, F64};

                debug("receiving #%" PRIu32 " globals\n", quantity_globals);
                for (uint32_t q = 0; q < quantity_globals; q++) {
                    uint8_t typeidx = *program_state++;
                    if (typeidx >= sizeof(valtypes)) {
                        debug("received unknown type %" PRIu8 "\n", typeidx);
                        exit(55);
                    }
                    StackValue *sv = &m->globals[m->global_count++];
                    size_t qb = typeidx == 0 || typeidx == 2 ? 4 : 8;
                    debug("receiving type %" PRIu8 " and %d bytes \n", typeidx,
                          typeidx == 0 || typeidx == 2 ? 4 : 8);

                    sv->value_type = valtypes[typeidx];
                    memcpy(&sv->value, program_state, qb);
                    program_state += qb;
                }
                break;
            }
            case tblState: {
                printf("receiving table\n");
                uint8_t tbl_type =
                    (uint8_t)*program_state++;  // for now only funcref
                uint32_t quantity = read_B32(&program_state);
                for (size_t i = 0; i < quantity; i++) {
                    uint32_t ne = read_B32(&program_state);
                    m->table.entries[m->table.size++] = ne;
                }
                break;
            }
            case memState: {
                debug("receiving memory\n");
                printf("receiving memory\n");
                uint32_t begin = read_B32(&program_state);
                uint32_t end = read_B32(&program_state);
                debug("memory offsets begin=%" PRIu32 " , end=%" PRIu32 "\n",
                      begin, end);
                if (begin > end) {
                    debug("incorrect memory offsets\n");
                    exit(57);
                }
                uint32_t totalbytes = end - begin + 1;
                uint8_t *mem_end =
                    m->memory.bytes + m->memory.pages * (uint32_t)PAGE_SIZE;
                debug("will copy #%" PRIu32 " bytes\n", totalbytes);
                if ((m->bytes + begin) + totalbytes > mem_end) {
                    debug("memory overflow\n");
                    exit(57);
                }
                memcpy(m->memory.bytes + begin, program_state, totalbytes + 1);
                for (auto i = begin; i <= (begin + totalbytes - 1); i++) {
                    debug("GOT byte idx %" PRIu32 " =%" PRIu8 "\n", i,
                          m->memory.bytes[i]);
                }
                debug("outside the out\n");
                program_state += totalbytes;
                break;
            }
            case brtblState: {
                debug("receiving br_table\n");
                printf("receiving br_table\n");
                uint16_t beginidx = read_B16(&program_state);
                uint16_t endidx = read_B16(&program_state);
                debug("br_table offsets begin=%" PRIu16 " , end=%" PRIu16 "\n",
                      beginidx, endidx);
                if (beginidx > endidx) {
                    debug("incorrect br_table offsets\n");
                    exit(57);
                }
                if (endidx >= BR_TABLE_SIZE) {
                    debug("br_table overflow\n");
                    exit(57);
                }
                for (auto idx = beginidx; idx <= endidx; idx++) {
                    // FIXME speedup with memcpy?
                    uint32_t el = read_B32(&program_state);
                    m->br_table[idx] = el;
                }
                break;
            }
            case stackvalsState: {
                // FIXME the float does add numbers at the end. The extra
                // numbers are present in the send information when dump occurs
                printf("receiving stack\n");
                uint16_t quantity_sv = read_B16(&program_state);
                uint8_t valtypes[] = {I32, I64, F32, F64};
                for (size_t i = 0; i < quantity_sv; i++) {
                    uint8_t typeidx = *program_state++;
                    if (typeidx >= sizeof(valtypes)) {
                        debug("received unknown type %" PRIu8 "\n", typeidx);
                        exit(55);
                    }
                    m->sp += 1;
                    StackValue *sv = &m->stack[m->sp];
                    sv->value.uint64 = 0;  // init whole union to 0
                    size_t qb = typeidx == 0 || typeidx == 2 ? 4 : 8;
                    sv->value_type = valtypes[typeidx];
                    memcpy(&sv->value, program_state, qb);
                    program_state += qb;
                }
                break;
            }
            default: {
                debug("saveState: Reiceived unknown program state\n");
                exit(33);
            }
        }
    }
    uint8_t done = (uint8_t)*program_state;
    return done == (uint8_t)1;
}

// void dump_stack_values(Module *m) {
//     wa_flush();
//     wa_printf("STACK");
//     wa_printf(R"({"stack":[)");
//     char _value_str[256];
//     for (int i = 0; i <= m->sp; i++) {
//         auto v = &m->stack[i];
//         format_constant_value(_value_str, v);
//         wa_printf(R"({"idx":%d, %s}%s)", i, _value_str,
//                   (i == m->sp) ? "" : ",");
//     }
//     wa_printf("]}\n");
//     wa_flush();
// }

void doDumpLocals(Module *m) {
    fflush(stdout);
    printf("DUMP LOCALS!\n\n");
    fflush(stdout);
    int firstFunFramePtr = m->csp;
    while (m->callstack[firstFunFramePtr].block->block_type != 0) {
        firstFunFramePtr--;
        if (firstFunFramePtr < 0) {
            FATAL("Not in a function!");
        }
    }
    Frame *f = &m->callstack[firstFunFramePtr];
    printf(R"({"count":%u,"locals":[)", 0);
    fflush(stdout);  // FIXME: this is needed for ESP to propery print
    char _value_str[256];
    for (size_t i = 0; i < f->block->local_count; i++) {
        auto v = &m->stack[m->fp + i];
        switch (v->value_type) {
            case I32:
                snprintf(_value_str, 255, R"("type":"i32","value":%)" PRIi32,
                         v->value.uint32);
                break;
            case I64:
                snprintf(_value_str, 255, R"("type":"i64","value":%)" PRIi64,
                         v->value.uint64);
                break;
            case F32:
                snprintf(_value_str, 255, R"("type":"i64","value":%.7f)",
                         v->value.f32);
                break;
            case F64:
                snprintf(_value_str, 255, R"("type":"i64","value":%.7f)",
                         v->value.f64);
                break;
            default:
                snprintf(_value_str, 255,
                         R"("type":"%02x","value":"%)" PRIx64 "\"",
                         v->value_type, v->value.uint64);
        }

        printf("{%s}%s", _value_str,
               (i + 1 < f->block->local_count) ? "," : "");
    }
    printf("]}\n\n");
    fflush(stdout);
}

/**
 * Read the change in bytes array.
 *
 * The array should be of the form
 * [0x10, index, ... new function body 0x0b]
 * Where index is the index without imports
 */
bool readChange(Module *m, uint8_t *bytes) {
    // Check if this was a change request
    if (*bytes != interruptUPDATEFun) return false;

    // SKIP the first byte (0x10), type of change
    uint8_t *pos = bytes + 1;

    uint32_t b = read_LEB_32(&pos);  // read id

    Block *function = &m->functions[m->import_count + b];
    uint32_t body_size = read_LEB_32(&pos);
    uint8_t *payload_start = pos;
    uint32_t local_count = read_LEB_32(&pos);
    uint8_t *save_pos;
    uint32_t tidx, lidx, lecount;

    // Local variable handling

    // Get number of locals for alloc
    save_pos = pos;
    function->local_count = 0;
    for (uint32_t l = 0; l < local_count; l++) {
        lecount = read_LEB_32(&pos);
        function->local_count += lecount;
        tidx = read_LEB(&pos, 7);
        (void)tidx;  // TODO: use tidx?
    }

    if (function->local_count > 0) {
        function->local_value_type =
            (uint8_t *)acalloc(function->local_count, sizeof(uint8_t),
                               "function->local_value_type");
    }

    // Restore position and read the locals
    pos = save_pos;
    lidx = 0;
    for (uint32_t l = 0; l < local_count; l++) {
        lecount = read_LEB_32(&pos);
        uint8_t vt = read_LEB(&pos, 7);
        for (uint32_t i = 0; i < lecount; i++) {
            function->local_value_type[lidx++] = vt;
        }
    }

    function->start_ptr = pos;
    function->end_ptr = payload_start + body_size - 1;
    function->br_ptr = function->end_ptr;
    ASSERT(*function->end_ptr == 0x0b, "Code section did not end with 0x0b\n");
    pos = function->end_ptr + 1;
    return true;
}

/**
 * Read change to local
 * @param m
 * @param bytes
 * @return
 */
bool readChangeLocal(Module *m, uint8_t *bytes) {
    if (*bytes != interruptUPDATELocal) return false;
    uint8_t *pos = bytes + 1;
    printf("Local updates: %x\n", *pos);
    uint32_t localId = read_LEB_32(&pos);

    printf("Local %u being cahnged\n", localId);
    auto v = &m->stack[m->fp + localId];
    switch (v->value_type) {
        case I32:
            v->value.uint32 = read_LEB_signed(&pos, 32);
            break;
        case I64:
            v->value.int64 = read_LEB_signed(&pos, 64);
            break;
        case F32:
            memcpy(&v->value.uint32, pos, 4);
            break;
        case F64:
            memcpy(&v->value.uint64, pos, 8);
            break;
    }
    printf("Local %u changed to %u\n", localId, v->value.uint32);
    return true;
}

/**
 * Validate if there are interrupts and execute them
 *
 * The various kinds of interrups are preceded by an identifier:
 *
 * - `0x01` : Continue running
 * - `0x02` : Halt the execution
 * - `0x03` : Pause execution
 * - `0x04` : Execute one operaion and then pause
 * - `0x06` : Add a breakpoint, the adress is specified as a pointer.
 *            The pointer should be specified as: 06[length][pointer]
 *            eg: 06 06 55a5994fa3d6
 * - `0x07` : Remove the breakpoint at the adress specified as a pointer if it
 *            exists (see `0x06`)
 * - `0x10` : Dump information about the program
 * - `0x11` :                  show locals
 * - `0x20` : Replace the content body of a function by a new function given
 *            as payload (immediately following `0x10`), see #readChange
 */
bool check_interrupts(RmvModule *rm, RunningState *program_state) {
#ifndef Arduino
#ifdef SOCKET
    processIncomingEvents();
    if (receivedDataSize() > 0) {
        auto *data = (uint8_t *)getReceivedData();
        rm->m->warduino->handleInterrupt(receivedDataSize(), data);
        freeReceivedData();
    }
#endif
#endif
    // FIXME before doing dangerous interrupts e.g. interruptStep, checp if
    // program state is PRoxy if so delay interrupt
    uint8_t *interruptData = nullptr;
    interruptData = rm->m->warduino->getInterrupt();
    if (interruptData) {
        switch (*interruptData) {
            case interruptRUN:
                printf("received run interrupt\n");
                wa_printf("GO!\n");
                wa_flush();
                if (*program_state == WARDUINOpause &&
                    rm->m->warduino->isBreakpoint(rm->m->pc_ptr)) {
                    rm->m->warduino->skipBreakpoint = rm->m->pc_ptr;
                }
                *program_state = WARDUINOrun;
                free(interruptData);
                break;
            case interruptHALT:
                wa_printf("STOP!\n");
                debug("STOP!\n");
                free(interruptData);
                exit(0);
                break;
            case interruptPAUSE:
                *program_state = WARDUINOpause;
                wa_printf("PAUSE!\n");
                printf("PAUSE!\n");
                debug("PAUSE!\n");
                free(interruptData);
                break;
            case interruptSTEP: {
                if (*program_state == WARDUINOpause &&
                    rm->m->warduino->isBreakpoint(rm->m->pc_ptr)) {
                    rm->m->warduino->skipBreakpoint = rm->m->pc_ptr;
                }
                debug("STEP!\n");
                printf("STEP\n");
                *program_state = WARDUINOstep;
                free(interruptData);
                wa_printf("STEP!\n");
                wa_flush();
                break;
            }
            case interruptUntil:
            case interruptBPAdd:  // Breakpoint
            case interruptBPRem:  // Breakpoint remove
            {
                // TODO: segfault may happen here!
                uint8_t len = interruptData[1];
                uintptr_t bp = 0x0;
                for (size_t i = 0; i < len; i++) {
                    bp <<= sizeof(uint8_t) * 8;
                    bp |= interruptData[i + 2];
                }
                auto *bpt = (uint8_t *)bp;
                if(*interruptData == 0x05){
                    printf("Until!\n");
                    wa_printf("Until %p!\n", static_cast<void *>(bpt));
                }
                else{
                    printf("BP received!\n");
                    wa_printf("BP %p!\n", static_cast<void *>(bpt));
                }
                wa_flush();

                if (*interruptData == 0x06)
                    rm->m->warduino->addBreakpoint(bpt);
                else if(*interruptData == 0x07)
                    rm->m->warduino->delBreakpoint(bpt);
                else{
                    *program_state = WARDUINOstep;
                    rm->m->warduino->until_pc = bpt;
                    rm->m->warduino->until_csp = rm->m->csp;
                }
                free(interruptData);

                break;
            }

            case interruptState:
                *program_state = WARDUINOpause;
                free(interruptData);
                doDump(rm);
                break;
            // case interruptDUMPStack:
            //     *program_state = WARDUINOpause;
            //     free(interruptData);
            //     dump_stack_values(rm->m);
            //     break;
            case interruptUPDATEFun:
                printf("CHANGE local!\n");
                debug("CHANGE local!\n");
                readChange(rm->m, interruptData);
                break;
            case interruptUPDATELocal:
                // printf("CHANGE local!\n");
                debug("CHANGE local!\n");
                readChangeLocal(rm->m, interruptData);
                free(interruptData);
                break;
            case interruptRecvState: {
                if (!receivingData) {
                    debug("paused program execution\n");
                    *program_state = WARDUINOpause;
                    receivingData = true;
                    freeState(rm->m, interruptData);
                    free(interruptData);
                    wa_printf("ack!\n");
                    wa_flush();
                } else {
                    printf("reeiving state\n");
                    debug("receiving state\n");
                    receivingData = !saveState(rm->m, interruptData);
                    free(interruptData);
                    debug("sending %s!\n", receivingData ? "ack" : "done");
                    wa_printf("%s!\n", receivingData ? "ack" : "done");
                    wa_flush();
                    if(!receivingData){
                        printf("receiving state done\n");
                    }
                }
                break;
            }
            case interruptOffset: {
                free(interruptData);
                wa_printf("\"{\"offset\":\"%p\"}\"\n", (void *)rm->m->bytes);
                wa_flush();
                break;
            }
            case interruptUPDATEMOD: {
                if (receivingData) {
                    uint8_t *data = interruptData + 1;
                    rm->byte_count = read_B32(&data);
                    rm->new_bytes =
                        (uint8_t *)malloc(sizeof(uint8_t) * rm->byte_count);
                    // FIXME might be missing one byte so rm->byte_count + 1 !!
                    memcpy(rm->new_bytes, data, rm->byte_count);
                    receivingData = false;
                    *program_state = WARDuinorestart;
                    rm->m->pc_error = nullptr; //TODO remove?
                    wa_printf("done!\n");
                    printf("Module updated\n");
                    wa_flush();
                } else {
                    receivingData = true;
                    free(interruptData);
                    wa_printf("ack!\n");
                    wa_flush();
                }
                break;
            }
            #ifndef Arduino
            case interruptMonitorProxies: {
                printf("receiving functions list to proxy\n");
                uint8_t *data = interruptData + 1;
                registerRFCs(rm->m, &data);
                registerHost(&data);
                free(interruptData);
                ProxyHost *host = ProxyHost::getProxyHost();
                if(!host->openConnection()){
                    printf("problem opening socket: %s\n", host->exceptionMsg);
                    exit(33);
                }
                wa_printf("done!\n");
                break;
            }
            case interruptRFCUseCache: {
                printf("RFC enabling cache use for list\n");
                uint8_t *data = interruptData + 1;
                setCacheRFCs(rm->m, &data, true);
                free(interruptData);
                break;
            }
            case interruptRFCNoCache: {
                printf("RFC disabling cache use for list\n");
                uint8_t *data = interruptData + 1;
                setCacheRFCs(rm->m, &data, false);
                free(interruptData);
                break;
            }
            #else
            case interruptProxyCall: {
                uint8_t *data = interruptData + 1;
                uint32_t fidx = read_L32(&data);
                printf("Call func %" PRIu32 "\n", fidx);

                Block *func = &rm->m->functions[fidx];
                StackValue *args = readRFCArgs(func, data);
                /* printf("Registering %" PRIu32 "as Callee\n", func->fidx); */
                RFC::registerRFCallee(fidx, func->type, args);

                free(interruptData);
                break;
            }
            #endif
            #ifdef Arduino
            case interruptToggleWiFi: {
                printf("Toggle WiFi\n");
                toggleWiFiConnection();
                break;
            }
            #endif
            default:
                // handle later
                printf("COULD not parse interrupt data!\n");
                free(interruptData);
                break;
        }
        return true;
    }
    return false;
}



void registerRFCs(Module * m, uint8_t **data){
    printf("registering_rfc_functions\n");
    RFC::clearRFCs();

    uint32_t amount_funcs = read_B32(data);
    printf("funcs_total %" PRIu32 "\n", amount_funcs);
    for (uint32_t i = 0; i < amount_funcs; i++) {
        uint32_t fid = read_B32(data);
        /* printf("registering fid=%" PRIu32 "\n", fid); */
        Type * type = (m->functions[fid]).type;
        RFC::registerRFC(fid, type);
    }
}

void setCacheRFCs(Module * m, uint8_t **data, bool cache){
    printf("changing RFC cache to %s\n", cache ? "true" : "false");

    uint32_t amount_funcs = read_B32(data);
    printf("funcs_total %" PRIu32 "\n", amount_funcs);
    for (uint32_t i = 0; i < amount_funcs; i++) {
        uint32_t fid = read_B32(data);
        printf("cache for %" PRIu32 "\n", fid);
        RFC* rfc = RFC::getRFC(fid);
        if(rfc != nullptr)
          rfc->useCache = cache;
    }
}

void registerHost(uint8_t **data) {
    int portno = (int)read_B32(data);
    uint8_t hostsize = (uint8_t)(*data)[0];
    char *hostname = new char[hostsize + 1];
    memcpy((void *)hostname, ++(*data), hostsize);
    hostname[hostsize] = '\0';
    /* printf("Registering Proxy Host: %s PORT=%" PRIu8 "\n", hostname, portno); */
    ProxyHost::getProxyHost()->registerHost(hostname, portno);
}

StackValue *readRFCArgs(Block *func, uint8_t *data) {
    if (func->type->param_count == 0){
        /* printf("ProxyFunc %" PRIu32 "takes no arg\n", func->fidx); */
        return nullptr;
    }

    StackValue *args = new StackValue[func->type->param_count];
    uint32_t *params = func->type->params;
    for (uint32_t i = 0; i < func->type->param_count; i++) {
        args[i].value.uint64 = 0;  // init whole union to 0
        args[i].value_type = params[i];

        switch (params[i]) {
            case I32: {
                memcpy(&args[i].value.uint32, data, sizeof(uint32_t));
                data += sizeof(uint32_t);
                /* printf("arg %d: i32 value %" PRIu32 "\n", i, */
                /*        args[i].value.uint32); */
                break;
            }
            case F32: {
                memcpy(&args[i].value.f32, data, sizeof(float));
                data += sizeof(float);
                /* printf("arg %d: F32 value %.7f \n", i, args[i].value.f32); */
                break;
            }
            case I64: {
                memcpy(&args[i].value.uint64, data, sizeof(uint64_t));
                data += sizeof(uint64_t);
                /* printf("arg %d: I64 value %" PRIu64 "\n", i, */
                /*        args[i].value.uint64); */
                break;
            }
            case F64: {
                memcpy(&args[i].value.f64, data, sizeof(double));
                data += sizeof(double);
                /* printf("arg %d: f64 value %.7f \n", i, args[i].value.f64); */
                break;
            }
            default: {
                printf("found a weird type!\n");
                exit(33);
                break;
            }
        }
    }
    return args;
}
