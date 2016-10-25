#include <stdio.h>
#include "time.h"

#define ASSERT_EQUAL(a, b, msg) \
    if (a != b) { \
        printf("%s: %f != %f\n", msg, a, b); \
        return 1; \
    }

#define ASSERT_WITHIN(a, b, slop, msg) \
    if (a - b > slop || b - a > slop) { \
        printf("%s: %f != %f\n", msg, a, b); \
        return 1; \
    }

int test_time_delta() {
    rage_Time a = {.fraction = UINT32_MAX * 0.1};
    ASSERT_EQUAL(rage_time_delta(a, a), 0.0, "Same time")
    rage_Time b = {.second = 1, .fraction = UINT32_MAX * 0.2};
    ASSERT_WITHIN(rage_time_delta(b, a), 1.1, 1E-6, "Ahead")
    ASSERT_WITHIN(rage_time_delta(a, b), -1.1, 1E-6, "Behind")
    return 0;
}

int test_time_after() {
    rage_Time a = {.second = 2, .fraction = 200};
    if (rage_time_after(a, a)) {
        printf("same\n");
        return 1;
    }
    rage_Time b = {.second = 1, .fraction = 100};
    if (!rage_time_after(a, b)) {
        printf("later\n");
        return 1;
    }
    if (rage_time_after(b, a)) {
        printf("earlier\n");
        return 1;
    }
    a.second = 1;
    if (!rage_time_after(a, b)) {
        printf("later frac\n");
        return 1;
    }
    if (rage_time_after(b, a)) {
        printf("earlier frac\n");
        return 1;
    }
    return 0;
}

int main() {
    int rv = test_time_delta();
    rv += test_time_after();
    return rv;
}
