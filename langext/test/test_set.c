#include "set.h"

rage_Error test_set() {
    rage_Error err = RAGE_OK;
    rage_Set * s0 = rage_set_new();
    rage_Set * s1 = rage_set_add(s0, 3);
    if (rage_set_contains(s0, 3)) {
        err = RAGE_ERROR("Original set mutated");
    } else if (!rage_set_contains(s1, 3)) {
        err = RAGE_ERROR("New set does not contain added value");
    } else if (rage_set_is_weak_subset(s1, s0)) {
        err = RAGE_ERROR("Claimed non-empty set to be subset of set with item");
    } else if (!rage_set_is_weak_subset(s0, s1)) {
        err = RAGE_ERROR("Claimed empty set wasn't subset of set with item");
    } else {
        rage_set_free(s0);
        s0 = rage_set_add(s1, 4);
        rage_set_free(s1);
        s1 = rage_set_add(s0, 5);
        if (!rage_set_contains(s1, 3)) {
            err = RAGE_ERROR("Lost an old value during add");
        } else {
            rage_set_free(s0);
            s0 = rage_set_remove(s1, 4);
            if (rage_set_contains(s0, 4)) {
                err = RAGE_ERROR("4 remains after removal");
            } else if (!rage_set_contains(s0, 3)) {
                err = RAGE_ERROR("Lost 3 removing 4");
            } else if (!rage_set_contains(s0, 5)) {
                err = RAGE_ERROR("Lost 5 removing 4");
            }
        }
    }
    rage_set_free(s0);
    rage_set_free(s1);
    return err;
}
