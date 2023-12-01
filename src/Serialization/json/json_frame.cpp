#include "json_frame.h"

#include "../../Utils/macros.h"
#include "../../Utils/util.h"

void JSON_serialize_Frame(const Channel& channel, const Frame* frame,
                          int frame_index, const Module* mod) {
    Module* m = (Module*)mod;
    auto toVA = [m](uint8_t* addr) { return toVirtualAddress(addr, m); };
    uint8_t bt = frame->block->block_type;
    uint32_t block_key = (bt == 0 || bt == 0xff || bt == 0xfe)
                             ? 0
                             : toVA(Module_findOpcode(m, frame->block));
    uint32_t fidx = bt == 0 ? frame->block->fidx : 0;
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
    channel.write(
        "{\"type\":%u,\"fidx\":\"0x%x\",\"sp\":%d,\"fp\":%d,\"idx\":%d,\"block_"
        "key\":%" PRIu32 ",\"ra\":%d,\"start\":%" PRIu32 ",\"callsite\":%d}",
        bt, fidx, frame->sp, frame->fp, frame_index, block_key, retaddr,
        start_addr, callsite_retaddr);
}

void JSON_deserialize_Frame(uint8_t* src, Frame* dest_frame) {
    FATAL("Frame json deserialization not implemented\n");
}
