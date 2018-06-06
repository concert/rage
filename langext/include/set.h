#pragma once
#include <stdlib.h>
#include <stdbool.h>

typedef struct rage_Set rage_Set;
typedef struct rage_SetElem rage_SetElem;

rage_Set * rage_set_new();
void rage_set_free(rage_Set * s);

bool rage_set_contains(rage_Set * s, rage_SetElem * v);

rage_Set * rage_set_add(rage_Set * s, rage_SetElem * v);
rage_Set * rage_set_remove(rage_Set * s, rage_SetElem * v);

bool rage_set_is_weak_subset(rage_Set * s0, rage_Set * s1);

#define RAGE_SET_OPS(name, lower, ty) \
    typedef struct rage_##name##Set rage_##name##Set; \
    static rage_##name##Set * rage_##lower##_set_new() { \
        return (rage_##name##Set *) rage_set_new(); \
    } \
    static void rage_##lower##_set_free(rage_##name##Set * s) { \
        return rage_set_free((rage_Set *) s); \
    } \
    static bool rage_##lower##_set_contains(rage_##name##Set * s, ty v) { \
        return rage_set_contains((rage_Set *) s, (rage_SetElem *) v); \
    } \
    static rage_##name##Set * rage_##lower##_set_add(rage_##name##Set * s, ty v) { \
        return (rage_##name##Set *) rage_set_add((rage_Set *) s, (rage_SetElem *) v); \
    } \
    static rage_##name##Set * rage_##lower##_set_remove(rage_##name##Set * s, ty v) { \
        return (rage_##name##Set *) rage_set_remove((rage_Set *) s, (rage_SetElem *) v); \
    } \
    static bool rage_##lower##_set_is_weak_subset(rage_##name##Set * s0, rage_##name##Set * s1) { \
        return rage_set_is_weak_subset((rage_Set *) s0, (rage_Set *) s1); \
    }
