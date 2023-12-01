#include "binary_multiple_objects.h"

struct JSON_PRINT_CONFIG {
    bool serializingMultipleObjects;
    bool objectPrinted;
} JSON_config;

void JSON_preSerialization(const Channel& channel) {
    if (!JSON_config.serializingMultipleObjects) {
        channel.write("{");
    } else {
        if (JSON_config.objectPrinted) {
            channel.write(",");
        }
    }
}

void JSON_postSerialization(const Channel& channel) {
    if (!JSON_config.serializingMultipleObjects) {
        channel.write("}\n");
    } else {
        JSON_config.objectPrinted = true;
    }
}

void JSON_startMultipleStructsSerialization(const Channel& channel) {
    JSON_config.serializingMultipleObjects = true;
    JSON_config.objectPrinted = false;
    channel.write("{");
}

void JSON_endMultipleStructsSerialization(const Channel& channel) {
    JSON_config.serializingMultipleObjects = false;
    JSON_config.objectPrinted = false;
    channel.write("}\n");
}