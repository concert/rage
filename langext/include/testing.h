#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"

typedef rage_Error (*rage_TestFunc)();

/*
 * Produce a TAP compatible test running main() that runs the rage_TestFunc
 * tests passed as arguments.
 */
#define TEST_MAIN(...) \
    int main() { \
        rage_TestFunc const tests[] = {__VA_ARGS__}; \
        unsigned const n_tests = sizeof(tests) / sizeof(rage_TestFunc); \
        char * test_names = strdup(#__VA_ARGS__); \
        const char * const delimiters = ", "; \
        char * saveptr; \
        char * test_name = strtok_r(test_names, delimiters, &saveptr); \
        printf("1..%u\n", n_tests); \
        for (unsigned i=0; i < n_tests; i++) { \
            rage_Error e = tests[i](); \
            if (RAGE_FAILED(e)) { \
                printf("not ok %u %s\n", i, test_name); \
                printf("Bail Out! %s\n", RAGE_FAILURE_VALUE(e)); \
                return 1; \
            } else { \
                printf("ok %u %s\n", i, test_name); \
            } \
            test_name = strtok_r(NULL, delimiters, &saveptr); \
        } \
        free(test_names); \
    }
