#include "error.h"
#include "countdown.h"

static void incr(void * v) {
    (*((int *) v))++;
}

rage_Error test_countdown() {
    int i = 0;
    rage_Countdown * c = rage_countdown_new(2, incr, (void *) &i);
    char * rval = NULL;
    if (i != 0) {
        rval = "Action ran before trigger";
    } else {
        rage_countdown_add(c, -1);
        if (i != 0) {
            rval = "Triggered early";
        } else {
            rage_countdown_add(c, -1);
            if (i != 1) {
                rval = "Did not trigger action";
            }
        }
    }
    rage_countdown_free(c);
    if (rval != NULL) {
        return RAGE_ERROR(rval);
    } else {
        return RAGE_OK;
    }
}
