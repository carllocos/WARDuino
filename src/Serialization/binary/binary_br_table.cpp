#include "binary_br_table.h"

#include "../../Structs/state_kind.h"
#include "../../Utils/macros.h"
#include "../../Utils/util.h"

void BINARY_serialize_BranchingTable(const Channel& channel,
                                     const uint32_t* br_table) {
    size_t encodedSize = 1 + size_for_LEB32(BR_TABLE_SIZE);
    for (uint32_t j = 0; j < BR_TABLE_SIZE; j++) {
        encodedSize += size_for_LEB32(br_table[j]);
    }
    uint8_t* encoding = (uint8_t*)malloc(encodedSize);
    if (encoding == nullptr) {
        FATAL("Memory allocation failed for Table struct");
    }

    encoding[0] = branchingTableState;
    size_t offset = 1;
    for (uint32_t j = 0; j < BR_TABLE_SIZE; j++) {
        offset += writeLEB32(br_table[j], encoding + offset);
    }

    for (size_t i = 0; i < encodedSize; i++) {
        channel.write("%02X ", encoding[i]);
    }
    free(encoding);
}

void BINARY_deserialize_BranchingTable(uint8_t* src, uint32_t* br_table) {
    FATAL("Branching Table json deserialization not implemented\n");
}
