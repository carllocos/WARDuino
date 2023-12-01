#pragma once
#include "../../Utils/sockets.h"

void JSON_preSerialization(const Channel& channel);

void JSON_postSerialization(const Channel& channel);

void JSON_startMultipleStructsSerialization(const Channel& channel);

void JSON_endMultipleStructsSerialization(const Channel& channel);
