#include <stdlib.h>
#include <stdio.h>
#include "error.h"
#include "chronology.h"

static rage_Error test_time_after() {
    rage_Time a = {.second = 2, .fraction = 200};
    if (rage_time_after(a, a))
        return RAGE_ERROR("Same time");
    rage_Time b = {.second = 1, .fraction = 100};
    if (!rage_time_after(a, b))
        return RAGE_ERROR("Later");
    if (rage_time_after(b, a))
        return RAGE_ERROR("Earlier");
    a.second = 1;
    if (!rage_time_after(a, b))
        return RAGE_ERROR("Later frac");
    if (rage_time_after(b, a))
        return RAGE_ERROR("Earlier frac");
    return RAGE_OK;
}
