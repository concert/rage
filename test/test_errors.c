#include <stdio.h>
#include <stdbool.h>
#include "error.h"

typedef RAGE_OR_ERROR(int) err_or_int;

static err_or_int may_fail(bool does_fail) {
    if (does_fail) {
        RAGE_FAIL(err_or_int, "sad times");
    } else {
        RAGE_SUCCEED(err_or_int, 3);
    }
}

typedef RAGE_OR_ERROR(float) err_or_float;

static err_or_float wrapper(bool does_fail) {
    err_or_int e = may_fail(does_fail);
    RAGE_EXTRACT_VALUE(err_or_float, e, int, i)
    RAGE_SUCCEED(err_or_float, i/2.0);
}

static void peof(err_or_float e) {
    if (RAGE_FAILED(e)) {
        printf("err: %s\n", RAGE_FAILURE_VALUE(e));
    } else {
        printf("val: %f\n", RAGE_SUCCESS_VALUE(e));
    }
}

int main() {
    peof(wrapper(true));
    peof(wrapper(false));
}
