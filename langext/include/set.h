#pragma once
#include <stdlib.h>
#include <stdbool.h>

typedef struct rage_Set rage_Set;

rage_Set * rage_set_new();
void rage_set_free(rage_Set * s);

bool rage_set_contains(rage_Set * s, int v);

rage_Set * rage_set_add(rage_Set * s, int v);
rage_Set * rage_set_remove(rage_Set * s, int v);

bool rage_set_is_weak_subset(rage_Set * s0, rage_Set * s1);
