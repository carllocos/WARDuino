#pragma once

#include <cstdint>

typedef struct LogicalClock {
    uint32_t nr_of_instructions{};
    uint32_t nr_of_events{};
} LogicalClock;

bool LogicalClock_is_t1_smaller_t2(const LogicalClock &t1,
                                   const LogicalClock &t2);

bool LogicalClock_is_t1_greater_t2(const LogicalClock &t1,
                                   const LogicalClock &t2);

bool LogicalClock_is_t1_equal_t2(const LogicalClock &t1,
                                 const LogicalClock &t2);
