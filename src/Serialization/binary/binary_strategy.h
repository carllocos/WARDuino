#pragma once

#include "../serialization_strategy.h"

void JSON_initialize_strategy(SerializationStrategy& strategy);

void JSON_initialize_serializer(SerializationStrategy& strategy);

void JSON_initialize_deserializer(SerializationStrategy& strategy);
