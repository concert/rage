#include "testing.h"
#include "refcounter.h"

static void sub1(int * i) {
    (*i)--;
}

typedef struct ActuallyInt ActuallyInt;
typedef RAGE_HS_COUNTABLE(int, ActuallyInt) RcInt;

static rage_Error test_refcounter() {
    int a = 1, b = 1;
    RAGE_HS_COUNT(ra, RcInt, sub1, &a);
    int * i = RAGE_HS_REF(ra);
    if (*i != a) {
        return RAGE_ERROR("Did not initialise correctly");
    }
    RAGE_HS_COUNT(rb, RcInt, sub1, &b);
    RAGE_HS_DEPEND_REF(rb, ra);
    RAGE_HS_DECREMENT_REF(ra);
    if (a != 1 || b != 1) {
        return RAGE_ERROR("deallocator invoked early");
    }
    RAGE_HS_DECREMENT_REF(rb);
    if (a || b) {
        return RAGE_ERROR("deallocator not invoked");
    }
    return RAGE_OK;
}

TEST_MAIN(test_refcounter)
