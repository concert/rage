#include "countdown.h"
#include <stdatomic.h>
#include <semaphore.h>
#include <stdlib.h>
#include <time.h>

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

rage_Error rage_countdown_timed_wait(rage_Countdown * c, unsigned millis) {
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
        RAGE_ERROR("Unable to get current time")
    }
    ts.tv_sec += millis / 1000;
    ts.tv_nsec += (millis % 1000) * 1000000;
    if (ts.tv_nsec > 1000000000) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000;
    }
    if (sem_timedwait(&c->sig, &ts) == -1) {
        RAGE_ERROR("Timed out waiting")
    }
    RAGE_OK
}

int rage_countdown_add(rage_Countdown * c, int delta) {
    int val = atomic_fetch_add(&c->counter, delta) + delta;
    if (val == 0) {
        sem_post(&c->sig);
    }
    return val;
}
