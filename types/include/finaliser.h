#pragma once

typedef struct rage_Finaliser rage_Finaliser;
typedef void (*rage_finalise_cb)(void *);

rage_Finaliser * rage_finaliser_new(rage_finalise_cb f, void * state);
void rage_finaliser_wait(rage_Finaliser * f);
