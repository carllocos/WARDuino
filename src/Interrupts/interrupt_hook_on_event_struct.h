#pragma once

#include "../Instrumentation/hook.h"

enum HookEventMoment {
    HookOnNewEvent = 0x01,
    HookOnEventHandling = 0x02,
    HookAfterEventHandled = 0x03
};

typedef struct OnEventHookRequest {
    HookEventMoment moment{};
    Hook *hook;
} OnEventHookRequest;
