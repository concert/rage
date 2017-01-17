#pragma once

typedef struct rage_Finaliser rage_Finaliser;
typedef void (*rage_finalise_cb)(void *);

rage_Finaliser * rage_finaliser_new(
    void * state, rage_finalise_cb f, unsigned n_waiters);
void rage_finaliser_done(rage_Finaliser * f);
void rage_finaliser_wait(rage_Finaliser * f);
