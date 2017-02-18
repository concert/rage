#include "finaliser.h"
#include <semaphore.h>
#include <stdlib.h>

struct rage_Finaliser {
    rage_finalise_cb finaliser;
    void * state;
};

rage_Finaliser * rage_finaliser_new(rage_finalise_cb f, void * state) {
    rage_Finaliser * r = malloc(sizeof(rage_Finaliser));
    r->state = state;
    r->finaliser = f;
    return r;
}

void rage_finaliser_wait(rage_Finaliser * f) {
    f->finaliser(f->state);
    free(f);
}
