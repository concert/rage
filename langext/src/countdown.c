#include "countdown.h"
#include <stdatomic.h>
#include <stdlib.h>


struct rage_Countdown {
    _Atomic int counter;
    rage_CountdownAction action;
    void * args;
};

_Static_assert(
    ATOMIC_INT_LOCK_FREE, "Atomic int not lock free on this platform");

rage_Countdown * rage_countdown_new(
        int initial_value, rage_CountdownAction action, void * args) {
    rage_Countdown * c = malloc(sizeof(rage_Countdown));
    atomic_init(&c->counter, initial_value);
    c->action = action;
    c->args = args;
    return c;
}

void rage_countdown_free(rage_Countdown * c) {
    free(c);
}

int rage_countdown_add(rage_Countdown * c, int delta) {
    int val = atomic_fetch_add(&c->counter, delta) + delta;
    if (val == 0) {
        c->action(c->args);
    }
    return val;
}

int rage_countdown_force_action(rage_Countdown * c) {
    int val = atomic_exchange(&c->counter, 0);
    if (val != 0) {
        c->action(c->args);
    }
    return val;
}
