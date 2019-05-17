#include "error.h"
#include "countdown.h"

static void incr(void * v) {
    (*((int *) v))++;
}

static rage_Error countdown_checks(rage_Countdown * c, int * i) {
    if (*i != 0) {
        return RAGE_ERROR("Action ran before trigger");
    }
    rage_countdown_add(c, -1);
    if (*i != 0) {
        return RAGE_ERROR("Triggered early");
    }
    rage_countdown_add(c, -1);
    if (*i != 1) {
        return RAGE_ERROR("Did not trigger action");
    }
    return RAGE_OK;
}

rage_Error test_countdown() {
    int i = 0;
    rage_Countdown * c = rage_countdown_new(2, incr, (void *) &i);
    rage_Error rval = countdown_checks(c, &i);
    rage_countdown_free(c);
    return rval;
}
