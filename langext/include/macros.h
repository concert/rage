#pragma once
#include <stdint.h>

typedef enum {
    RAGE_EITHER_LEFT,
    RAGE_EITHER_RIGHT
} rage_EitherHalf;

#define RAGE_EITHER(a, b) \
    struct { \
        rage_EitherHalf half; \
        union { \
            a left; \
            b right; \
        }; \
    }

#define RAGE_MAYBE(t) RAGE_EITHER(void *, t)
#define RAGE_JUST(v) {.half = RAGE_EITHER_RIGHT, .right = v}
#define RAGE_NOTHING {.half = RAGE_EITHER_LEFT, .left = NULL}

#define RAGE_ARRAY(t) \
    struct { \
        uint32_t len; \
        t * items; \
    }

#define RAGE_ARRAY_INIT(var, length, index) \
    (var)->len = length; \
    (var)->items = calloc(sizeof(__typeof__(*(var)->items)), (var)->len); \
    for (uint32_t index=0; index < (var)->len; index++)
