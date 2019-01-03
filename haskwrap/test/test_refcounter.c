#include "testing.h"
#include "refcounter.h"

static void sub1(int * i) {
    (*i)--;
}

static rage_Error test_refcounter() {
    int a = 1, b = 1;
    rage_hs_RefCount * ra = rage_hs_count_ref((rage_hs_Deallocator) sub1, &a);
    rage_hs_RefCount * rb = rage_hs_count_ref((rage_hs_Deallocator) sub1, &b);
    rage_hs_depend_ref(rb, ra);
    rage_hs_decrement_ref(ra);
    if (a != 1 || b != 1) {
        return RAGE_ERROR("deallocator invoked early");
    }
    rage_hs_decrement_ref(rb);
    if (a || b) {
        return RAGE_ERROR("deallocator not invoked");
    }
    return RAGE_OK;
}

TEST_MAIN(test_refcounter)
