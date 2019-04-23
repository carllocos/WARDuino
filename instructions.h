#include "WARDuino.h"
#include <stdbool.h>

void push_block(Module *m, Block *block, int sp);
Block *pop_block(Module *m) ;

void setup_call(Module *m, uint32_t fidx);

/*
 * WebAssembly Instructions
 *
 * i_instr_**** functions
 * 
 * Functions here are called from a dispatching loop
 * in WARDuino.cpp. 
 * 
 * The given module `m` is the currently executing module
 * 
 * Returning false breaks this loop and marks the 
 * execution as crashed. The exception varaible can
 * be filled with an explaination.
 * 
 * Returning true continues the loop with the
 * notable exception of end wich can terminate
 * the progran successfully by setting program_done
 * 
 */

bool i_instr_block(Module *m, uint32_t *cur_pc);
bool i_instr_loop(Module *m, uint32_t *cur_pc);
bool i_instr_if(Module *m, uint32_t *cur_pc);
bool i_instr_else(Module *m, uint32_t *cur_pc);
bool i_instr_end(Module *m, uint32_t *cur_pc, bool* prog_done);
bool i_instr_br(Module *m, uint32_t *cur_pc);
bool i_instr_br_if(Module *m, uint32_t *cur_pc);
bool i_instr_br_table(Module *m, uint32_t *cur_pc);
bool i_instr_return(Module *m, uint32_t *cur_pc);
bool i_instr_call(Module *m, uint32_t *cur_pc);
bool i_instr_call_indirect(Module *m, uint32_t *cur_pc);
bool i_instr_drop(Module *m, uint32_t *cur_pc);
bool i_instr_select(Module *m, uint32_t *cur_pc);
bool i_instr_get_local(Module *m, uint32_t *cur_pc);
bool i_instr_set_local(Module *m, uint32_t *cur_pc);
bool i_instr_tee_local(Module *m, uint32_t *cur_pc);
bool i_instr_set_global(Module *m, uint32_t *cur_pc);
bool i_instr_current_memory(Module *m, uint32_t *cur_pc);
bool i_instr_grow_memory(Module *m, uint32_t *cur_pc);
bool i_instr_mem_load(Module *m, uint32_t *cur_pc, uint8_t opcode);
bool i_instr_mem_store(Module *m, uint32_t *cur_pc, uint8_t opcode);
bool i_instr_const(Module *m, uint32_t *cur_pc, uint8_t opcode);
bool i_instr_unairy_u32(Module *m, uint32_t *cur_pc, uint8_t opcode);
bool i_instr_math_u32(Module *m, uint32_t *cur_pc, uint8_t opcode);
bool i_instr_math_u64(Module *m, uint32_t *cur_pc, uint8_t opcode);
bool i_instr_math_f32(Module *m, uint32_t *cur_pc, uint8_t opcode);
bool i_instr_math_f64(Module *m, uint32_t *cur_pc, uint8_t opcode);
bool i_instr_unairy_i32(Module *m, uint32_t *cur_pc, uint8_t opcode);
bool i_instr_unairy_i64(Module *m, uint32_t *cur_pc, uint8_t opcode);
bool i_instr_unairy_floating(Module *m, uint32_t *cur_pc, uint8_t opcode);
bool i_instr_binary_i32(Module *m, uint32_t *cur_pc, uint8_t opcode);
bool i_instr_binary_i64(Module *m, uint32_t *cur_pc, uint8_t opcode);
bool i_instr_binary_f32(Module *m, uint32_t *cur_pc, uint8_t opcode);
bool i_instr_binary_f64(Module *m, uint32_t *cur_pc, uint8_t opcode);
bool i_instr_conversion(Module *m, uint32_t *cur_pc, uint8_t opcode);