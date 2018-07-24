#include "set.h"

RAGE_SET_OPS(Int, int, long)
RAGE_SET_REMOVAL_OPS(Int, int, long)

rage_Error test_set() {
    rage_Error err = RAGE_OK;
    rage_IntSet * s0 = rage_int_set_new();
    rage_IntSet * s1 = rage_int_set_add(s0, 3);
    if (rage_int_set_contains(s0, 3)) {
        err = RAGE_ERROR("Original set mutated");
    } else if (!rage_int_set_contains(s1, 3)) {
        err = RAGE_ERROR("New set does not contain added value");
    } else if (rage_int_set_is_weak_subset(s1, s0)) {
        err = RAGE_ERROR("Claimed non-empty set to be subset of empty set");
    } else if (!rage_int_set_is_weak_subset(s0, s1)) {
        err = RAGE_ERROR("Claimed empty set wasn't subset of set with item");
    } else {
        rage_int_set_free(s0);
        s0 = rage_int_set_add(s1, 4);
        rage_int_set_free(s1);
        s1 = rage_int_set_add(s0, 5);
        if (!rage_int_set_contains(s1, 3)) {
            err = RAGE_ERROR("Lost an old value during add");
        } else {
            rage_int_set_free(s0);
            s0 = rage_int_set_remove(s1, 4);
            if (rage_int_set_contains(s0, 4)) {
                err = RAGE_ERROR("4 remains after removal");
            } else if (!rage_int_set_contains(s0, 3)) {
                err = RAGE_ERROR("Lost 3 removing 4");
            } else if (!rage_int_set_contains(s0, 5)) {
                err = RAGE_ERROR("Lost 5 removing 4");
            }
            rage_IntSet * s2 = rage_int_set_subtract(s1, s0);
            if (rage_int_set_contains(s2, 3) || rage_int_set_contains(s2, 5)) {
                err = RAGE_ERROR("Subtract contains common value");
            } else if (!rage_int_set_contains(s2, 4)) {
                err = RAGE_ERROR("Subtract missing differing value");
            }
            rage_int_set_free(s2);
        }
    }
    rage_int_set_free(s0);
    rage_int_set_free(s1);
    return err;
}
