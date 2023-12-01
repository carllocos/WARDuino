#pragma once
#include "../../Utils/sockets.h"

void BINARY_preSerialization(const Channel& channel);

void BINARY_postSerialization(const Channel& channel);

void BINARY_startMultipleStructsSerialization(const Channel& channel);

void BINARY_endMultipleStructsSerialization(const Channel& channel);
