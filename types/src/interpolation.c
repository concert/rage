#include <stdlib.h>
#include "interpolation.h"

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

// FIXME: Different file for the above

static void rage_float_interpolate(
        rage_Atom * const target, rage_Atom const * const start,
        rage_Atom const * const end, const float weight) {
    target->f = (start->f * (1 - weight)) + (end->f * weight);
}

static void rage_int_interpolate(
        rage_Atom * const target, rage_Atom const * const start,
        rage_Atom const * const end, const float weight) {
    target->i = (start->i * (1 - weight)) + (end->i * weight);
}

rage_InitialisedInterpolator rage_interpolator_new(rage_TupleDef type) {
    rage_Interpolator state = malloc(sizeof(rage_Interpolator_));
    state->value = calloc(sizeof(rage_Atom), type.len);
    state->interpolators = calloc(sizeof(rage_AtomInterpolator *), type.len);
    for (unsigned i=0; i < type.len; i++) {
        rage_AtomDef const * atom_def = type.items[i].type;
        switch (atom_def->type) {
            case (RAGE_ATOM_FLOAT):
                state->interpolators[i] = rage_float_interpolate;
                break;
            case (RAGE_ATOM_INT):
                state->interpolators[i] = rage_int_interpolate;
                break;
            case (RAGE_ATOM_TIME):
            case (RAGE_ATOM_STRING):
                rage_interpolator_free(state);
                RAGE_FAIL(rage_InitialisedInterpolator, "String interpolation is not supported");
        }
    }
    state->len = type.len;
    RAGE_SUCCEED(rage_InitialisedInterpolator, state);
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
        if (rage_time_after(points.items[i].time, time)) {
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
            state->interpolators[i](
                state->value + i, start->value + i, end->value + i, weight);
        }
        return state->value;
    }
}
