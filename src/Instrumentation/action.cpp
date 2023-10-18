#include "action.h"

Action *Actions_add_and_sort(Action *actions, Action *action_to_add) {
    if (action_to_add == nullptr) {
        return actions;
    } else if (actions == nullptr) {
        return action_to_add;
    }

    // TODO sort actions from more specific to more general
    // sorting goes from timestamp, ev dep, cond, once, always
    // timestamp < timestamp < .. < timestamp
    // -> sub sort ts before(s) .... ts on ts .... ts after
    // event dependency < event dep. < ... < event dep. condition
    // Once < once < ... < once
    // always (only one allowed)
    Action *insert = actions;
    while (insert->nextAction != nullptr) {
        insert = insert->nextAction;
    }
    insert->nextAction = action_to_add;
}

Action *Actions_nextScheduledAction(Action *sorted_actions,
                                    const TimeStamp &currentTime) {
    // sorted actions go from timestamp, ev dep, cond, once, always
    // timestamp < timestamp < .. < timestamp
    // -> sub sort ts before(s) .... ts on ts .... ts after
    // event dependency < event dep. < ... < event dep. condition
    // Once < once < ... < once
    // always (only one allowed)

    Action *action = sorted_actions;
    while (action != nullptr) {
        switch (action->schedule.kind) {
            case ScheduleBeforeTimeStamp:
                // TODO: decide whether before makes sense
                // ScheduleBeforeTimeStamp means that the action should
                // be scheduled to run before the current timestamp becomes
                // equal to the timestamp assigned to the action
                if (TimeStamp_is_t1_smaller_t2(
                        currentTime, action->schedule.value.timeStamp)) {
                    return action;
                }
                break;
            case ScheduleOnTimeStamp:
                if (TimeStamp_is_t1_equal_t2(action->schedule.value.timeStamp,
                                             currentTime)) {
                    return action;
                }
                break;
            case ScheduleAfterTimeStamp:
                // ScheduleAfterTimeStamp means that the action should
                // be scheduled to run only after the current timestamp becomes
                // greater than the timestamp assigned to the action
                if (TimeStamp_is_t1_greater_t2(
                        currentTime, action->schedule.value.timeStamp)) {
                    return action;
                }
                break;
            case ScheduleOnce:
                return action;
            case ScheduleAlways:
                return action;
        }
        action = action->nextAction;
    }
    return nullptr;
}

bool Actions_isActionWaitingForEvent(Action *sorted_actions,
                                     const TimeStamp &currentTime) {
    Action *action = sorted_actions;
    while (action != nullptr) {
        // TODO decide: whether to move the code here to Sceduler.h
        switch (action->schedule.kind) {
            case ScheduleOnTimeStamp:
            case ScheduleAfterTimeStamp:
            case ScheduleBeforeTimeStamp:
                return action->schedule.value.timeStamp.nr_of_events >
                           currentTime.nr_of_events &&
                       action->schedule.value.timeStamp.nr_of_instructions >=
                           currentTime.nr_of_instructions;
            default:
                action = action->nextAction;
        }
    }
    return false;
}

Action *Actions_copyAction(const Action &action) {
    Action *cpy = new Action();
    if (cpy != nullptr) {
        cpy->kind = action.kind;
        cpy->schedule = action.schedule;
        switch (cpy->kind) {
            case RemoteCall:
                cpy->value.target_fidx = action.value.target_fidx;
                break;
            case ValueSubstitution:
                if (action.value.result != nullptr) {
                    cpy->value.result = new StackValue;
                    cpy->value.result->value = action.value.result->value;
                    cpy->value.result->value_type =
                        action.value.result->value_type;
                }
                break;
            default:
                return nullptr;
        }
    }
    return cpy;
}

void Actions_free_action(Action *action) {
    action->nextAction = nullptr;
    if (action->kind == ValueSubstitution && action->value.result != nullptr) {
        delete action->value.result;
    }
    delete action;
}
