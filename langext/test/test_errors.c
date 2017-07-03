#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "error.h"

typedef RAGE_OR_ERROR(int) err_or_int;

static err_or_int may_fail(bool does_fail) {
    if (does_fail)
        return RAGE_FAIL(err_or_int, "sad times");
    return RAGE_SUCCEED(err_or_int, 3);
}

typedef RAGE_OR_ERROR(float) err_or_float;

static err_or_float wrapper(bool does_fail) {
    err_or_int e = may_fail(does_fail);
    RAGE_EXTRACT_VALUE(err_or_float, e, int i)
    return RAGE_SUCCEED(err_or_float, i/2.0);
}

static rage_Error basic(bool does_fail) {
    if (does_fail)
        RAGE_ERROR("at life")
    RAGE_OK
}

int main() {
    int rval = 0;
    printf("1..3\n");
    err_or_float e = wrapper(false);
    if (RAGE_FAILED(e)) {
        printf("not ok 0 Unexpectedly failed: %s\n", RAGE_FAILURE_VALUE(e));
        rval = 1;
    } else {
        if (RAGE_SUCCESS_VALUE(e) != 1.5) {
            printf("not ok 0 Incorrect success value %f != 1.5\n", RAGE_SUCCESS_VALUE(e));
            rval = 2;
        } else {
            printf("ok 0 wrapped success\n");
        }
    }
    e = wrapper(true);
    if (!RAGE_FAILED(e)) {
        printf("not ok 1 Unexpectedly succeeded: %f\n", RAGE_SUCCESS_VALUE(e));
        rval = 3;
    } else {
        if (strcmp(RAGE_FAILURE_VALUE(e), "sad times") != 0) {
            printf("not ok 1 Incorrect failure value %s != \"sad times\"\n", RAGE_FAILURE_VALUE(e));
            rval = 4;
        } else {
            printf("ok 1 wrapped failure\n");
        }
    }
    rage_Error err = basic(false);
    if (RAGE_FAILED(err)) {
        printf("not ok 2 Basic unexpectedly failed\n");
        rval = 5;
    } else {
        printf("ok 2 basic success\n");
    }
    err = basic(true);
    if (!RAGE_FAILED(err)) {
        printf("not ok 3 Basic unexpectedly succeeded\n");
        rval = 6;
    } else {
        if (strcmp(RAGE_FAILURE_VALUE(err), "at life") != 0) {
            printf("not ok 3 Basic incorrect failure value %s\n", RAGE_FAILURE_VALUE(err));
            rval = 7;
        } else {
            printf("ok 3 basic failure\n");
        }
    }
    return rval;
}
