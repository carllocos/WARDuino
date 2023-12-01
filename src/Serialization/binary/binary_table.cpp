#include "binary_table.h"

#include <inttypes.h>

#include "../../Structs/state_kind.h"
#include "../../Utils/macros.h"
#include "../../Utils/util.h"
#include "binary_multiple_objects.h"

uint8_t *tableToLEB32(const Table *table, size_t *size) {
    // Output format: state_type, elem_type, initial, maximum, size,
    // [entries]
    size_t encodedSize = 1 + 1 + size_for_LEB32(table->initial) +
                         size_for_LEB32(table->maximum) +
                         size_for_LEB32(table->size);
    for (uint32_t i = 0; i < table->size; i++) {
        encodedSize += writeLEB32(table->entries[i], nullptr);
    }

    // Allocate memory for the LEB32 encoded data
    uint8_t *encodedData = (uint8_t *)malloc(encodedSize);

    if (encodedData == nullptr) {
        // Memory allocation failed
        *size = 0;
        return nullptr;
    }

    // Encode kind of state
    encodedData[0] = tableState;

    // Encode elem_type
    encodedData[1] = table->elem_type;
    size_t offset = 2;

    // Encode initial, maximum, and size
    offset += writeLEB32(table->initial, encodedData + offset);
    offset += writeLEB32(table->maximum, encodedData + offset);
    offset += writeLEB32(table->size, encodedData + offset);

    // Encode entries
    for (uint32_t i = 0; i < table->size; i++) {
        offset += writeLEB32(table->entries[i], encodedData + offset);
    }

    *size = encodedSize;
    return encodedData;
}

void BINARY_serialize_Table(const Channel &channel, const Table &table) {
    size_t encodedSize;
    uint8_t *encodedData = tableToLEB32(&table, &encodedSize);

    if (encodedData == nullptr) {
        FATAL("Memory allocation failed for Table struct");
    }
    for (size_t i = 0; i < encodedSize; i++) {
        channel.write("%02X ", encodedData[i]);
    }
    free(encodedData);
}

void BINARY_deserialize_Table(uint8_t *src, const Table &table) {
    FATAL("Table json deserialization not implemented\n");
}

int main() {}
