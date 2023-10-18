#pragma once

#include <cstdint>

typedef struct {
    uint32_t nr_of_instructions{};
    uint32_t nr_of_events{};
} TimeStamp;

bool TimeStamp_is_t1_smaller_t2(const TimeStamp &t1, const TimeStamp &t2);

bool TimeStamp_is_t1_greater_t2(const TimeStamp &t1, const TimeStamp &t2);

bool TimeStamp_is_t1_equal_t2(const TimeStamp &t1, const TimeStamp &t2);
