#pragma once
#include <stdint.h>
#include <stdlib.h>

/*
 * Enumeration for identifying which half of an either type is currently in
 * use.
 */
typedef enum {
    RAGE_EITHER_LEFT,
    RAGE_EITHER_RIGHT
} rage_EitherHalf;

/*
 * Hakell inspired parameterised either type.
 */
#define RAGE_EITHER(a, b) \
    struct { \
        rage_EitherHalf half; \
        union { \
            a left; \
            b right; \
        }; \
    }

/*
 * Haskell inspired parameterised option type.
 */
#define RAGE_MAYBE(t) RAGE_EITHER(void *, t)
/*
 * Haskell inspired Just (option present in maybe) value.
 */
#define RAGE_JUST(v) {.half = RAGE_EITHER_RIGHT, .right = v}
/*
 * Haskell inspired Nothing (empty maybe) value.
 */
#define RAGE_NOTHING {.half = RAGE_EITHER_LEFT, .left = NULL}

/*
 * Parameterised array type.
 */
#define RAGE_ARRAY(t) \
    struct { \
        uint32_t len; \
        t * items; \
    }

/*
 * Factored out array initialisation boilerplate.
 * Honestly it's 3 lines, you're best off reading it.
 */
#define RAGE_ARRAY_INIT(var, length, index) \
    (var)->len = length; \
    (var)->items = calloc(sizeof(__typeof__(*(var)->items)), (var)->len); \
    for (uint32_t index=0; index < (var)->len; index++)
