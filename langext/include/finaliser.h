#pragma once

/*
 * A mechanism for delayed finalisation (for deferred operations).
 * Think this may need another round of design.
 */
typedef struct rage_Finaliser rage_Finaliser;
typedef void (*rage_finalise_cb)(void *);

/*
 * Initialise a finaliser (on the heap) with a "closure-like" state thing.
 */
rage_Finaliser * rage_finaliser_new(rage_finalise_cb f, void * state);
/*
 * Wait for the finaliser to complete (actually runs the finaliser too).
 */
void rage_finaliser_wait(rage_Finaliser * f);
