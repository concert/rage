#pragma once

#include <stdio.h>
#include "error.h"

typedef rage_Error (*rage_TestFunc)();

#define TEST_MAIN(...) \
    int main() { \
        rage_TestFunc const tests[] = {__VA_ARGS__}; \
        unsigned const n_tests = sizeof(tests) / sizeof(rage_TestFunc); \
        for (unsigned i=0; i < n_tests; i++) { \
            rage_Error e = tests[i](); \
            if (RAGE_FAILED(e)) { \
                printf("Failed: %s\n", RAGE_FAILURE_VALUE(e)); \
                return 1; \
            } \
        } \
    }
