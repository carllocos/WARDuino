#pragma once
#include "./logical_clock.h"

enum ScheduleKind {
    ScheduleOnce = 0x01,            // hook run once and as soon as possible
    ScheduleAlways = 0x03,          // hook runs everytime
    ScheduleOnLogicalClock = 0x21,  // hook executed on logical-clock
    ScheduleBeforeLogicalClock =
        0x22,  // hook executed before given logical-clock
    ScheduleAfterLogicalClock = 0x23  // hook executed after given logical-clock
};

typedef struct Schedule {
    ScheduleKind kind;
    union {
        LogicalClock logicalClock{};
    } value;
} Schedule;
