#pragma once
#include "serialization_strategy.h"

enum SerializationFormat { JSON_Serialization, BINARY_Serialization };

void Serialization_factory(SerializationStrategy& strategy,
                           SerializationFormat format);