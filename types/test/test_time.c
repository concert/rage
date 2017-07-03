#include <stdlib.h>
#include <stdio.h>
#include "testing.h"
#include "error.h"
#include "time.h"

typedef struct {
    unsigned size;
    char * buf;
} StringBuffer;

int error_context(rage_Error (*f)(StringBuffer b)) {
    const unsigned buf_size = 80;
    char buf[buf_size];
    StringBuffer sb = {.buf=buf, .size=buf_size};
    rage_Error e = f(sb);
    if (RAGE_FAILED(e)) {
        printf("%s\n", RAGE_FAILURE_VALUE(e));
        return 1;
    }
    return 0;
}

#define ASSERT_EQUAL(a, b, msg) \
    if (a != b) { \
        snprintf(eb.buf, eb.size, "%s: %f != %f", msg, a, b); \
        return RAGE_ERROR(eb.buf); \
    }

#define ASSERT_WITHIN(a, b, slop, msg) \
    if (a - b > slop || b - a > slop) { \
        snprintf(eb.buf, eb.size, "%s: %f != %f", msg, a, b); \
        return RAGE_ERROR(eb.buf); \
    }

rage_Error test_time_delta(StringBuffer eb) {
    rage_Time a = {.fraction = UINT32_MAX * 0.1};
    ASSERT_EQUAL(rage_time_delta(a, a), 0.0, "Same time")
    rage_Time b = {.second = 1, .fraction = UINT32_MAX * 0.2};
    ASSERT_WITHIN(rage_time_delta(b, a), 1.1, 1E-6, "Ahead")
    ASSERT_WITHIN(rage_time_delta(a, b), -1.1, 1E-6, "Behind")
    return RAGE_OK;
}

rage_Error test_time_after() {
    rage_Time a = {.second = 2, .fraction = 200};
    if (rage_time_after(a, a))
        return RAGE_ERROR("Same time");
    rage_Time b = {.second = 1, .fraction = 100};
    if (!rage_time_after(a, b))
        return RAGE_ERROR("Later");
    if (rage_time_after(b, a))
        return RAGE_ERROR("Earlier");
    a.second = 1;
    if (!rage_time_after(a, b))
        return RAGE_ERROR("Later frac");
    if (rage_time_after(b, a))
        return RAGE_ERROR("Earlier frac");
    return RAGE_OK;
}

TEST_MAIN(test_time_after)
