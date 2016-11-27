#include <stdlib.h>
#include "time_series.h"

rage_TimeSeries rage_time_series_new(rage_TupleDef const * const item_def) {
    // FIXME: really crappy and hard coded
    rage_TimeSeries ts;
    ts.len = 1;
    rage_TimePoint * tp = malloc(sizeof(rage_TimePoint));
    tp->time.second = 0;
    tp->time.fraction = 0;
    tp->value = malloc(sizeof(rage_Atom));
    tp->value[0].f = 0;
    tp->mode = RAGE_INTERPOLATION_CONST;
    ts.items = tp;
    return ts;
}

void rage_time_series_free(rage_TimeSeries ts) {
    for (uint32_t i = 0; i < ts.len; i++) {
        free(ts.items[i].value);
    }
    free(ts.items);
}
