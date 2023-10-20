#pragma once
#include "../WARDuino/structs.h"
#include "./schedule.h"
#include "./timestamp.h"

#define ACTION_ERROR_CODE_SUBSTITUTE_VALUE_IS_MALFORMED 31;
#define ACTION_ERROR_CODE_UNEXISTING_ACTION_KIND 32;
#define ACTION_ERROR_CODE_UNEXISTING_SCHEDULE_KIND 33;

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

Action *Actions_remove_completed_action(Action *first_action,
                                        Action *action_completed);

void Actions_free_action(Action *action);

bool Actions_deserialize_action(Action &dest, uint8_t **encoded_action,
                                uint8_t &error_code);

bool Actions_deserialize_schedule(Schedule &dest, uint8_t **encoded_schedule,
                                  uint8_t &error_code);