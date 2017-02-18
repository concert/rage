#include "barrier.h"
#include <stdlib.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <assert.h>

_Static_assert(
    ATOMIC_CHAR_LOCK_FREE, "Atomic char not lock free on this platform");

struct rage_Barrier {
    atomic_uchar tokens;
    sem_t s;
};

rage_Barrier * rage_barrier_new(unsigned char tokens) {
    rage_Barrier * b = malloc(sizeof(rage_Barrier));
    sem_init(&b->s, 0, 0);
    atomic_init(&b->tokens, tokens);
    return b;
}

void rage_barrier_free(rage_Barrier * b) {
    // Possibly this whole block should be ifdeffed
    int semval;
    sem_getvalue(&b->s, &semval);
    assert(semval);
    // End of ifdeffable block
    sem_destroy(&b->s);
    free(b);
}

void rage_barrier_release_token(rage_Barrier * b) {
    if (atomic_fetch_sub_explicit(&b->tokens, 1, memory_order_relaxed) == 1) {
        sem_post(&b->s);
    }
}

void rage_barrier_wait(rage_Barrier * b) {
    sem_wait(&b->s);
}
