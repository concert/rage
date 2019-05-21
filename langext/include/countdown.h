#pragma once
#include "error.h"

typedef void (*rage_CountdownAction)(void * args);

/*
 * In some senses the opposite of a semaphore, it triggers the action when the
 * count reaches 0 (as opposed to blocking until it is positive).
 */
typedef struct rage_Countdown rage_Countdown;

/*
 * New countdown (on the heap), initial_value is the starting value of the
 * counter.
 */
rage_Countdown * rage_countdown_new(
    int initial_value, rage_CountdownAction action, void * args);
/*
 * Free rage_Countdown
 */
void rage_countdown_free(rage_Countdown * c);

/*
 * Add a value to the countdown (note the argument is signed).
 * This is a realtime safe operation.
 */
int rage_countdown_add(rage_Countdown * c, int delta);

/*
 * Push the countdown to the maximum possible delay.
 * Returns the countdown value after the operation.
 */
int rage_countdown_max_delay(rage_Countdown * c);

/*
 * Reset the countdown and trigger the action, returns previous value.
 */
int rage_countdown_force_action(rage_Countdown * c);
