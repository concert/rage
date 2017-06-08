#pragma once
#include "error.h"

/*
 * In some senses the opposite of a semaphore, it triggers when the count
 * reaches 0 (as opposed to blocking until it is positive).
 */
typedef struct rage_Countdown rage_Countdown;

/*
 * New countdown (on the heap), initial_value is the starting value of the
 * counter. New instances are in the "blocking" state.
 */
rage_Countdown * rage_countdown_new(int initial_value);
/*
 * Free rage_Countdown
 */
void rage_countdown_free(rage_Countdown * c);

/*
 * Wait for the countdown to hit 0 for up to the requested time in
 * milliseconds.
 */
rage_Error rage_countdown_timed_wait(rage_Countdown * c, unsigned millis);
/*
 * Add a value to the countdown (note the argument is signed).
 * This is a realtime safe operation.
 */
int rage_countdown_add(rage_Countdown * c, int delta);
