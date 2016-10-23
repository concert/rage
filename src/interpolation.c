#include <stdlib.h>
#include "interpolation.h"

rage_Interpolator rage_interpolator_new(rage_TupleDef type) {
    rage_Interpolator state = malloc(sizeof(rage_Interpolator_));
    state->value = calloc(sizeof(rage_Atom), type.len);
    state->len = type.len;
    return state;
}

void rage_interpolator_free(rage_Interpolator state) {
    free(state->value);
    free(state);
}

rage_Tuple rage_interpolate(rage_Interpolator state, rage_Time time, rage_TimeSeries points) {
    rage_TimePoint const * start = NULL;
    rage_TimePoint const * end = NULL;
    unsigned i;
    for (i = 0; i < points.len; i++) {
        if (rage_time_delta(points.items[i].time, time) > 0) {
            end = &(points.items[i]);
            break;
        }
        start = &(points.items[i]);
    }
    if (end == NULL || start->mode == RAGE_INTERPOLATION_CONST) {
        return start->value;
    } else {
        float const weight =
            rage_time_delta(time, start->time) /
            rage_time_delta(end->time, start->time);
        for (i = 0; i < state->len; i++) {
            // FIXME: float only!
            state->value[i].f =
                (start->value[i].f * (1 - weight)) +
                (end->value[i].f * weight);
        }
        return state->value;
    }
}
