#include "countdown.h"
#include <stdatomic.h>
#include <semaphore.h>
#include <stdlib.h>

struct rage_Countdown {
    _Atomic int counter;
    sem_t sig;
};

_Static_assert(
    ATOMIC_INT_LOCK_FREE, "Atomic int not lock free on this platform");

rage_Countdown * rage_countdown_new(int initial_value) {
    rage_Countdown * c = malloc(sizeof(rage_Countdown));
    atomic_init(&c->counter, initial_value);
    sem_init(&c->sig, 0, (initial_value == 0) ? 1 : 0);
    return c;
}

void rage_countdown_free(rage_Countdown * c) {
    sem_destroy(&c->sig);
    free(c);
}

void rage_countdown_wait(rage_Countdown * c) {
    sem_wait(&c->sig);
}

int rage_countdown_add(rage_Countdown * c, int delta) {
    int val = atomic_fetch_add(&c->counter, delta) + delta;
    if (val == 0) {
        sem_post(&c->sig);
    }
    return val;
}
