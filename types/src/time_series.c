#include <stdlib.h>
#include "time_series.h"

rage_TimeSeries rage_time_series_new(rage_TupleDef const * const item_def) {
    rage_TimePoint * tp = malloc(sizeof(rage_TimePoint));
    tp->time = (rage_Time) {.second = 0};
    tp->value = rage_tuple_generate(item_def);
    tp->value[0].f = 0;
    tp->mode = RAGE_INTERPOLATION_CONST;
    return (rage_TimeSeries) {.len = 1, .items = tp};
}

void rage_time_series_free(rage_TimeSeries ts) {
    for (uint32_t i = 0; i < ts.len; i++) {
        free(ts.items[i].value);
    }
    free(ts.items);
}
