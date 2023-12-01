#include "binary_frame.h"

#include "../../Utils/macros.h"
#include "../../Utils/util.h"
#define FUN_BLOCK 0
#define PROXY_GUARD_BLOCK 0xff
#define CALLBACK_GUARD_BLOCK 0xfe


size_t serialization_size_frame(uint8_t block_type, uint32_t fun_idx,
                                uint32_t block_key, int return_addr,
                                int callsite_retaddr, int start_addr, int sp,
                                int fp, int idx) {
    size_t encoding_size = 1;  // block type
    if (block_type == FUN_BLOCK) {
        encoding_size += size_for_LEB32(fun_idx);
    }

    if (block_type == FUN_BLOCK || block_type == PROXY_GUARD_BLOCK ||
        block_type == CALLBACK_GUARD_BLOCK) {
        encoding_size += size_for_LEB32(block_key);
    }
    encoding_size += size_for_int(fp);
    encoding_size += size_for_int(sp);
    encoding_size += size_for_int(idx);
    encoding_size += size_for_int(return_addr);
    encoding_size += size_for_int(callsite_retaddr);
    encoding_size += size_for_LEB32(start_addr);
    return encoding_size;
}

void BINARY_serialize_Frame(const Channel& channel, const Frame* frame,
                            int frame_index, const Module* mod) {
    Module* m = (Module*)mod;
    auto toVA = [m](uint8_t* addr) { return toVirtualAddress(addr, m); };
    uint8_t bt = frame->block->block_type;
    uint32_t block_key = (bt == FUN_BLOCK || bt == PROXY_GUARD_BLOCK ||
                          bt == CALLBACK_GUARD_BLOCK)
                             ? 0
                             : toVA(Module_findOpcode(m, frame->block));
    uint32_t fidx = bt == FUN_BLOCK ? frame->block->fidx : 0;
    int callsite_retaddr = -1;
    int retaddr = -1;
    if (frame->ra_ptr != nullptr) {
        // first frame has no retrun address
        callsite_retaddr =
            toVA(frame->ra_ptr - 2);  // callsite of function (if type 0)
        retaddr = toVA(frame->ra_ptr);
    }

    uint32_t start_addr =
        frame->block->start_ptr != nullptr ? toVA(frame->block->start_ptr) : 0;
    size_t encoding_size =
        serialization_size_frame(bt, fidx, block_key, retaddr, callsite_retaddr,
                                 start_addr, frame->sp, frame->fp, frame_index);
    uint8_t* encoding = (uint8_t*)malloc(encoding_size);
    if (encoding == nullptr) {
        FATAL("No memory left for BINARY_FRAME\n");
    }

    encoding[0] = bt;
    size_t offset = 1;
    if (bt == FUN_BLOCK) {
        offset += writeLEB32(fidx, encoding + offset);
    }
    offset += write_int(frame->sp, encoding + offset);
    offset += write_int(frame->fp, encoding + offset);
    offset += write_int(frame_index, encoding + offset);
    if (bt != FUN_BLOCK || bt != PROXY_GUARD_BLOCK ||
        bt != CALLBACK_GUARD_BLOCK) {
        offset += writeLEB32(block_key, encoding + offset);
    }
    offset += write_int(retaddr, encoding + offset);
    offset += writeLEB32(start_addr, encoding + offset);
    offset += write_int(callsite_retaddr, encoding + offset);
    for (size_t i = 0; i < encoding_size; i++) {
        channel.write("%02X ", encoding[i]);
    }
    free(encoding);
}

void BINARY_deserialize_Frame(uint8_t* src, Frame* dest_frame) {
    FATAL("Frame json deserialization not implemented\n");
}
