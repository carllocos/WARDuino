#include "serialization_factory.h"

#include "../Utils/macros.h"
#include "./json/json_strategy.h"

void Serialization_factory(SerializationStrategy& strategy,
                           SerializationFormat format) {
    switch (format) {
        case JSON_Serialization:
            JSON_initialize_strategy(strategy);
            break;
        default:
            FATAL("Serialization Format not supported");
    }
}
