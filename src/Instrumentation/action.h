#pragma once
#include "../WARDuino/structs.h"
#include "./schedule.h"
#include "./timestamp.h"

enum ActionKind { RemoteCall = 0x01, ValueSubstitution = 0x02 };

struct AroundAction {
    ActionKind kind;
    union {
        uint32_t target_fidx;
        StackValue *result{};
    } value;
    Schedule schedule;
    AroundAction *nextAction{};
};

/*
 * sorts all the actions based on the order they will be executed.
 * function assumes that `actions` are already sorted
 */
AroundAction *Actions_add_and_sort(AroundAction *actions,
                                   AroundAction *action_to_add);

AroundAction *Actions_nextScheduledAction(AroundAction *sorted_actions,
                                          const TimeStamp &currentTime);

bool Actions_isActionWaitingForEvent(AroundAction *sorted_actions,
                                     const TimeStamp &currentTime);

AroundAction *Actions_copyAction(const AroundAction &action);