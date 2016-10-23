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

int main() {
    return test_time_delta();
}
