#pragma once
#include "./timestamp.h"

enum ScheduleKind {
    ScheduleOnce = 0x01,             // action run once and as soon as possible
    ScheduleAlways = 0x03,           // action runs everytime
    ScheduleOnTimeStamp = 0x21,      // action executed on timestamp
    ScheduleBeforeTimeStamp = 0x22,  // action executed before given timestamp
    ScheduleAfterTimeStamp = 0x23    // action executed after given timestamp
};

typedef struct {
    ScheduleKind kind;
    union {
        TimeStamp timeStamp{};
    } value;
} Schedule;
