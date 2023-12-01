#include "json_table.h"

#include <inttypes.h>

#include "../../Utils/macros.h"
#include "json_multiple_objects.h"

void JSON_serialize_Table(const Channel& channel, const Table& table) {
    JSON_preSerialization(channel);
    channel.write(R"("table":{"max":%d, "init":%d, "elements":[)",
                  table.maximum, table.initial);
    for (uint32_t j = 0; j < table.size; j++) {
        channel.write("%" PRIu32 "%s", table.entries[j],
                      (j + 1) == table.size ? "" : ",");
    }
    channel.write("]}");
    JSON_postSerialization(channel);
}

void JSON_deserialize_Table(uint8_t* src, const Table& table) {
    FATAL("Table json deserialization not implemented\n");
}
