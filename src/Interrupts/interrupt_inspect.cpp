#include "interrupt_inspect.h"

#include "../Utils/macros.h"
#include "../Utils/util.h"

void printValue(const Channel &output, StackValue *v, uint32_t idx,
                bool end = false);

uint8_t *findOpcode(Module *m, Block *block);

void Interrupt_Inspect_handle_request(const Channel &requester, Module *m,
                                      uint8_t *encode_request) {
    uint8_t *data = encode_request + 1;
    uint16_t numberBytes = read_B16(&data);
    ExecutionState *state = (ExecutionState *)data;
    Interrupt_Inspect_inspect_json_output(requester, m, numberBytes, state);
}

void Interrupt_Inspect_inspect_json_output(const Channel &requester,
                                           const Module *m,
                                           uint16_t sizeStateArray,
                                           const ExecutionState *state) {
    debug("asked for inspect\n");
    uint16_t idx = 0;
    auto toVA = [m](uint8_t *addr) {
        return toVirtualAddress(addr, (Module *)m);
    };
    bool addComma = false;

    requester.write("DUMP!\n");
    requester.write("{");

    while (idx < sizeStateArray) {
        switch (state[idx++]) {
            case pcState: {  // PC
                requester.write("\"pc\":%" PRIu32 "", toVA(m->pc_ptr));
                addComma = true;

                break;
            }
            case breakpointsState: {
                requester.write("%s\"breakpoints\":[", addComma ? "," : "");
                addComma = true;
                size_t i = 0;
                for (auto bp : m->warduino->debugger->breakpoints) {
                    requester.write(
                        "%" PRIu32 "%s", toVA(bp),
                        (++i < m->warduino->debugger->breakpoints.size()) ? ","
                                                                          : "");
                }
                requester.write("]");
                break;
            }
            case callstackState: {
                requester.write("%s\"callstack\":[", addComma ? "," : "");
                addComma = true;
                for (int j = 0; j <= m->csp; j++) {
                    Frame *f = &m->callstack[j];
                    uint8_t bt = f->block->block_type;
                    uint32_t block_key =
                        (bt == 0 || bt == 0xff || bt == 0xfe)
                            ? 0
                            : toVA(findOpcode((Module *)m, f->block));
                    uint32_t fidx = bt == 0 ? f->block->fidx : 0;
                    int ra = f->ra_ptr == nullptr ? -1 : toVA(f->ra_ptr);
                    requester.write(
                        R"({"type":%u,"fidx":"0x%x","sp":%d,"fp":%d,"idx":%d,)",
                        bt, fidx, f->sp, f->fp, j);
                    requester.write("\"block_key\":%" PRIu32 ",\"ra\":%d}%s",
                                    block_key, ra, (j < m->csp) ? "," : "");
                }
                requester.write("]");
                break;
            }
            case stackState: {
                requester.write("%s\"stack\":[", addComma ? "," : "");
                addComma = true;
                for (int j = 0; j <= m->sp; j++) {
                    auto v = &m->stack[j];
                    printValue(requester, v, j, j == m->sp);
                }
                requester.write("]");
                break;
            }
            case globalsState: {
                requester.write("%s\"globals\":[", addComma ? "," : "");
                addComma = true;
                for (uint32_t j = 0; j < m->global_count; j++) {
                    auto v = m->globals + j;
                    printValue(requester, v, j, j == (m->global_count - 1));
                }
                requester.write("]");  // closing globals
                break;
            }
            case tableState: {
                requester.write(
                    R"(%s"table":{"max":%d, "init":%d, "elements":[)",
                    addComma ? "," : "", m->table.maximum, m->table.initial);
                addComma = true;
                for (uint32_t j = 0; j < m->table.size; j++) {
                    requester.write("%" PRIu32 "%s", m->table.entries[j],
                                    (j + 1) == m->table.size ? "" : ",");
                }
                requester.write("]}");  // closing table
                break;
            }
            case branchingTableState: {
                requester.write(R"(%s"br_table":{"size":"0x%x","labels":[)",
                                addComma ? "," : "", BR_TABLE_SIZE);
                for (uint32_t j = 0; j < BR_TABLE_SIZE; j++) {
                    requester.write("%" PRIu32 "%s", m->br_table[j],
                                    (j + 1) == BR_TABLE_SIZE ? "" : ",");
                }
                requester.write("]}");
                break;
            }
            case memoryState: {
                uint32_t total_elems = m->memory.pages * (uint32_t)PAGE_SIZE;
                requester.write(
                    R"(%s"memory":{"pages":%d,"max":%d,"init":%d,"bytes":[)",
                    addComma ? "," : "", m->memory.pages, m->memory.maximum,
                    m->memory.initial);
                addComma = true;
                for (uint32_t j = 0; j < total_elems; j++) {
                    requester.write("%" PRIu8 "%s", m->memory.bytes[j],
                                    (j + 1) == total_elems ? "" : ",");
                }
                requester.write("]}");  // closing memory
                break;
            }
            case callbacksState: {
                bool noOuterBraces = false;
                requester.write(
                    "%s%s", addComma ? "," : "",
                    CallbackHandler::dump_callbacksV2(noOuterBraces).c_str());
                addComma = true;
                break;
            }
            case eventsState: {
                requester.write("%s", addComma ? "," : "");
                m->warduino->debugger->dumpEvents(
                    0, CallbackHandler::event_count());
                addComma = true;
                break;
            }
            default: {
                debug("dumpExecutionState: Received unknown state request\n");
                break;
            }
        }
    }
    requester.write("}\n");
}
uint8_t *findOpcode(Module *m, Block *block) {
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

void printValue(const Channel &output, StackValue *v, uint32_t idx, bool end) {
    char buff[256];

    switch (v->value_type) {
        case I32:
            snprintf(buff, 255, R"("type":"i32","value":%)" PRIi32,
                     v->value.uint32);
            break;
        case I64:
            snprintf(buff, 255, R"("type":"i64","value":%)" PRIi64,
                     v->value.uint64);
            break;
        case F32:
            snprintf(buff, 255, R"("type":"F32","value":"%)" PRIx32 "\"",
                     v->value.uint32);
            break;
        case F64:
            snprintf(buff, 255, R"("type":"F64","value":"%)" PRIx64 "\"",
                     v->value.uint64);
            break;
        default:
            snprintf(buff, 255, R"("type":"%02x","value":"%)" PRIx64 "\"",
                     v->value_type, v->value.uint64);
    }
    output.write(R"({"idx":%d,%s}%s)", idx, buff, end ? "" : ",");
}
