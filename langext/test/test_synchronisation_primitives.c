#include "error.h"
#include "countdown.h"

rage_Error test_countdown() {
    rage_Countdown * c = rage_countdown_new(2);
    char * rval = NULL;
    rage_Error err = rage_countdown_timed_wait(c, 1);
    if (!RAGE_FAILED(err)) {
        rval = "Failed to block";
    }
    if (rval == NULL) {
        rage_countdown_add(c, -1);
        err = rage_countdown_timed_wait(c, 1);
        if (!RAGE_FAILED(err)) {
            rval = "Released early";
        }
    }
    if (rval == NULL) {
        rage_countdown_add(c, -1);
        err = rage_countdown_timed_wait(c, 1);
        if (RAGE_FAILED(err)) {
            rval = "Did not release";
        }
    }
    rage_countdown_free(c);
    if (rval != NULL) {
        return RAGE_ERROR(rval);
    } else {
        return RAGE_OK;
    }
}
