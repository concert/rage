#include "set.h"

struct rage_Set {
    unsigned usage_count;
    rage_Set * next;
    rage_SetElem * value;
};

rage_Set * rage_set_new() {
    return NULL;
}

void rage_set_free(rage_Set * s) {
    while (s != NULL) {
        if (--s->usage_count) {
            break;
        } else {
            rage_Set * next = s->next;
            free(s);
            s = next;
        }
    }
}

static rage_Set * rage_set_item_matching(rage_Set * s, rage_SetElem * v) {
    while (s != NULL) {
        if (s->value == v) {
            break;
        }
        s = s->next;
    }
    return s;
}

bool rage_set_contains(rage_Set * s, rage_SetElem * v) {
    return rage_set_item_matching(s, v) != NULL;
}

static rage_Set * rage_set_unchecked_add(rage_Set * s, rage_SetElem * v) {
    rage_Set * new = malloc(sizeof(rage_Set));
    new->value = v;
    new->next = s;
    new->usage_count = 1;
    return new;
}

rage_Set * rage_set_add(rage_Set * s, rage_SetElem * v) {
    if (s != NULL) {
        s->usage_count++;
    }
    if (rage_set_contains(s, v)) {
        return s;
    } else {
        return rage_set_unchecked_add(s, v);
    }
}

rage_Set * rage_set_remove(rage_Set * s, rage_SetElem * v) {
    rage_Set * matching = rage_set_item_matching(s, v);
    if (matching == NULL) {
        s->usage_count++;
        return s;
    }
    rage_Set * new = matching->next;
    new->usage_count++;
    while (s != matching) {
        new = rage_set_unchecked_add(new, s->value);
        s = s->next;
    }
    return new;
}

bool rage_set_is_weak_subset(rage_Set * s0, rage_Set * s1) {
    while (s0 != NULL) {
        if (!rage_set_contains(s1, s0->value)) {
            return false;
        }
        s0 = s0->next;
    }
    return true;
}
