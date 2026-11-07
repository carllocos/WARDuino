#pragma once
#include <cstdint>

/*
 * Wire format of a schedule: SCHEDULE_KIND (1 byte)
 *
 * A hook either runs every time (the default) or only once after which it is
 * removed. A hook only stores whether it runs once (see `Hook::once`).
 */
enum ScheduleKind : uint8_t {
    ScheduleOnce = 0x01,   // hook run once and as soon as possible
    ScheduleAlways = 0x03  // hook runs everytime
};
