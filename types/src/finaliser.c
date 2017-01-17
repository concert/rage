#include "finaliser.h"
#include <semaphore.h>
#include <stdlib.h>

struct rage_Finaliser {
    sem_t sem;
    void * state;
    rage_finalise_cb finaliser;
};

rage_Finaliser * rage_finaliser_new(
        void * state, rage_finalise_cb f, unsigned n_waiters) {
    rage_Finaliser * r = malloc(sizeof(rage_Finaliser));
    sem_init(&r->sem, 0, n_waiters);
    r->state = state;
    r->finaliser = f;
    return r;
}

void rage_finaliser_done(rage_Finaliser * f) {
    sem_post(&f->sem);
}

void rage_finaliser_wait(rage_Finaliser * f) {
    sem_wait(&f->sem);
    f->finaliser(f->state);
    sem_destroy(&f->sem);
    free(f);
}
