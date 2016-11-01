#include <stdlib.h>
#include <stdio.h>
#include "error.h"
#include "time.h"

typedef struct {
    unsigned size;
    char * buf;
} StringBuffer;

StringBuffer string_buffer_new() {
    const unsigned buf_size = 80;
    StringBuffer sb = {.size=buf_size, .buf=malloc(buf_size)};
    return sb;
}

void string_buffer_free(StringBuffer sb) {
    free(sb.buf);
}

int error_context(rage_Error (*f)(StringBuffer b)) {
    StringBuffer sb = string_buffer_new();
    rage_Error e = f(sb);
    int rval = 0;
    if (RAGE_FAILED(e)) {
        printf("%s\n", RAGE_FAILURE_VALUE(e));
        rval = 1;
    }
    string_buffer_free(sb);
    return rval;
}

#define ASSERT_EQUAL(a, b, msg) \
    if (a != b) { \
        snprintf(eb.buf, eb.size, "%s: %f != %f", msg, a, b); \
        RAGE_ERROR(eb.buf); \
    }

#define ASSERT_WITHIN(a, b, slop, msg) \
    if (a - b > slop || b - a > slop) { \
        snprintf(eb.buf, eb.size, "%s: %f != %f", msg, a, b); \
        RAGE_ERROR(eb.buf); \
    }

rage_Error test_time_delta(StringBuffer eb) {
    rage_Time a = {.fraction = UINT32_MAX * 0.1};
    ASSERT_EQUAL(rage_time_delta(a, a), 0.0, "Same time")
    rage_Time b = {.second = 1, .fraction = UINT32_MAX * 0.2};
    ASSERT_WITHIN(rage_time_delta(b, a), 1.1, 1E-6, "Ahead")
    ASSERT_WITHIN(rage_time_delta(a, b), -1.1, 1E-6, "Behind")
    RAGE_OK
}

rage_Error test_time_after() {
    rage_Time a = {.second = 2, .fraction = 200};
    if (rage_time_after(a, a))
        RAGE_ERROR("Same time")
    rage_Time b = {.second = 1, .fraction = 100};
    if (!rage_time_after(a, b))
        RAGE_ERROR("Later")
    if (rage_time_after(b, a))
        RAGE_ERROR("Earlier")
    a.second = 1;
    if (!rage_time_after(a, b))
        RAGE_ERROR("Later frac")
    if (rage_time_after(b, a))
        RAGE_ERROR("Earlier frac")
    RAGE_OK
}

int main() {
    int rv = error_context(test_time_delta);
    rage_Error err = test_time_after();
    if (RAGE_FAILED(err)) {
        printf("Time after failed: %s\n", RAGE_FAILURE_VALUE(err));
        rv += 1;
    }
    return rv;
}
