#include "logical_clock.h"

bool LogicalClock_is_t1_smaller_t2(const LogicalClock &t1,
                                   const LogicalClock &t2) {
    return (t1.nr_of_instructions <= t2.nr_of_instructions &&
            t1.nr_of_events < t2.nr_of_events) ||
           (t1.nr_of_events == t2.nr_of_events &&
            t1.nr_of_instructions < t2.nr_of_instructions);
}

bool LogicalClock_is_t1_greater_t2(const LogicalClock &t1,
                                   const LogicalClock &t2) {
    return (t1.nr_of_instructions == t2.nr_of_instructions &&
            t1.nr_of_events > t2.nr_of_events) ||
           (t1.nr_of_instructions > t2.nr_of_instructions &&
            t1.nr_of_events >= t2.nr_of_events);
}

bool LogicalClock_is_t1_equal_t2(const LogicalClock &t1,
                                 const LogicalClock &t2) {
    return t1.nr_of_instructions == t2.nr_of_instructions &&
           t1.nr_of_events == t2.nr_of_events;
}
