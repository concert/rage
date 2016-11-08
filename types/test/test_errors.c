#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "error.h"

typedef RAGE_OR_ERROR(int) err_or_int;

static err_or_int may_fail(bool does_fail) {
    if (does_fail)
        RAGE_FAIL(err_or_int, "sad times")
    RAGE_SUCCEED(err_or_int, 3)
}

typedef RAGE_OR_ERROR(float) err_or_float;

static err_or_float wrapper(bool does_fail) {
    err_or_int e = may_fail(does_fail);
    RAGE_EXTRACT_VALUE(err_or_float, e, int i)
    RAGE_SUCCEED(err_or_float, i/2.0)
}

static rage_Error basic(bool does_fail) {
    if (does_fail)
        RAGE_ERROR("at life")
    RAGE_OK
}

int main() {
    err_or_float e = wrapper(false);
    if (RAGE_FAILED(e)) {
        printf("Unexpectedly failed: %s\n", RAGE_FAILURE_VALUE(e));
        return 1;
    } else {
        if (RAGE_SUCCESS_VALUE(e) != 1.5) {
            printf("Incorrect success value %f != 1.5\n", RAGE_SUCCESS_VALUE(e));
            return 2;
        }
    }
    e = wrapper(true);
    if (!RAGE_FAILED(e)) {
        printf("Unexpectedly succeeded: %f\n", RAGE_SUCCESS_VALUE(e));
        return 3;
    } else {
        if (strcmp(RAGE_FAILURE_VALUE(e), "sad times") != 0) {
            printf("Incorrect failure value %s != \"sad times\"\n", RAGE_FAILURE_VALUE(e));
            return 4;
        }
    }
    rage_Error err = basic(false);
    if (RAGE_FAILED(err)) {
        printf("Basic unexpectedly failed\n");
        return 5;
    }
    err = basic(true);
    if (!RAGE_FAILED(err)) {
        printf("Basic unexpectedly succeeded\n");
        return 6;
    } else {
        if (strcmp(RAGE_FAILURE_VALUE(err), "at life") != 0) {
            printf("Basic incorrect failure value %s\n", RAGE_FAILURE_VALUE(err));
            return 7;
        }
    }
}
