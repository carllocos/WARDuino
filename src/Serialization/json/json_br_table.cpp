#include "json_br_table.h"

#include <inttypes.h>

#include "../../Utils/macros.h"
#include "../../WARDuino/WARDuino_constants.h"
#include "json_multiple_objects.h"

void JSON_serialize_BranchingTable(const Channel& channel,
                                   const uint32_t* br_table) {
    JSON_preSerialization(channel);
    channel.write(R"("br_table":{"size":"0x%x","labels":[)", BR_TABLE_SIZE);
    for (uint32_t j = 0; j < BR_TABLE_SIZE; j++) {
        channel.write("%" PRIu32 "%s", br_table[j],
                      (j + 1) == BR_TABLE_SIZE ? "" : ",");
    }
    channel.write("]}");
    JSON_postSerialization(channel);
}

void JSON_deserialize_BranchingTable(uint8_t* src, uint32_t* br_table) {
    FATAL("Branching Table json deserialization not implemented\n");
}
