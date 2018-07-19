#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
 * Check if maybe is justy
 */
#define RAGE_IS_JUST(v) (v.half == RAGE_EITHER_RIGHT)

#define RAGE_FROM_JUST(v) v.right

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

/*
 * Free a heap allocated array.
 */
#define RAGE_ARRAY_FREE(arr) free(arr->items); free(arr);

/*
 * Define a function to make an extended copy of an array.
 */
#define RAGE_POINTER_ARRAY_APPEND_FUNC_DEF(array_type, item_type, func_name) \
    array_type * func_name(array_type * old_array, item_type * item) { \
        array_type * new_array = malloc(sizeof(array_type)); \
        new_array->len = old_array->len + 1; \
        new_array->items = calloc(sizeof(item_type *), new_array->len); \
        memcpy(new_array->items, old_array->items, sizeof(item_type *) * old_array->len); \
        new_array->items[old_array->len] = item; \
        return new_array; \
    }

/*
 * Define a function to make a copy of an array with an element removed.
 */
#define RAGE_POINTER_ARRAY_REMOVE_FUNC_DEF(array_type, item_type, func_name) \
    array_type * func_name(array_type * old_array, item_type * item) { \
        array_type * new_array = malloc(sizeof(array_type)); \
        new_array->len = old_array->len - 1; \
        new_array->items = calloc(sizeof(item_type *), new_array->len); \
        uint32_t j = 0; \
        for (uint32_t i=0; i < old_array->len; i++) { \
            if (old_array->items[i] != item) { \
                new_array->items[j++] = old_array->items[i]; \
            } \
        } \
        return new_array; \
    }

/*
 * RAGE internally uses structs wrapped in anonymous unions to reduce
 * intermediary dots in accesors (without losing the ability to take a pointer
 * to the sub struct).
 * However should you be using a tool (e.g. c2hs) that doesn't understand this
 * then #define RAGE_DISABLE_ANON_UNIONS before including.
 */
#ifdef RAGE_DISABLE_ANON_UNIONS
#define RAGE_EMBED_STRUCT(t, v) t v
#else
#define RAGE_EMBED_STRUCT(t, v) union {t; t v;}
#endif
