#pragma once
#include "../WARDuino/structs.h"
#include "./schedule.h"
#include "./timestamp.h"

enum ActionKind { RemoteCall = 0x01, ValueSubstitution = 0x02 };

struct Action {
    ActionKind kind;
    union {
        uint32_t target_fidx;
        StackValue *result{};
    } value;
    Schedule schedule;
    Action *nextAction{};
};

/*
 * sorts all the actions based on the order they will be executed.
 * function assumes that `actions` are already sorted
 */
Action *Actions_add_and_sort(Action *actions, Action *action_to_add);

Action *Actions_nextScheduledAction(Action *sorted_actions,
                                    const TimeStamp &currentTime);

bool Actions_isActionWaitingForEvent(Action *sorted_actions,
                                     const TimeStamp &currentTime);

Action *Actions_copyAction(const Action &action);

void Actions_free_action(Action *action);