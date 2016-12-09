#include <stdlib.h>
#include <stdbool.h>
#include "interpolation.h"

static rage_FrameNo rage_time_to_frame(rage_Time const * t, uint32_t sample_rate) {
    return (sample_rate * t->second) + (sample_rate * (((float) t->fraction) / UINT32_MAX));
}

static bool unambiguous_mode(rage_InterpolationMode mode) {
    switch (mode) {
        case (RAGE_INTERPOLATION_CONST):
        case (RAGE_INTERPOLATION_LINEAR):
            return true;
    }
    return false;
}

typedef void (*rage_AtomInterpolator)(
    rage_Atom * const target, rage_Atom const * const start,
    rage_Atom const * const end, const float weight);

typedef struct {
    rage_FrameNo frame;
    rage_Atom * value;
    rage_InterpolationMode mode;
} rage_FramePoint;

typedef RAGE_ARRAY(rage_FramePoint) rage_FrameSeries;

struct rage_Interpolator {
    uint32_t len;
    rage_AtomInterpolator * interpolators;
    rage_Atom * value;
    rage_FrameNo pos;
    rage_FrameSeries points;
    // FIXME: transport state
};

static rage_Error validate_time_series(
        rage_TimeSeries const * points, rage_InterpolationMode allowed_modes) {
    if (points->len == 0) {
        RAGE_ERROR("No points in time series");
    }
    rage_TimePoint const * point = &points->items[0];
    rage_Time const * t = &point->time;
    if (t->second || t->fraction) {
        RAGE_ERROR("First value is not at start of time series");
    }
    for (uint32_t i = 0; i < points->len; i++) {
        point = &points->items[i];
        if (!(point->mode & allowed_modes && unambiguous_mode(point->mode))) {
            RAGE_ERROR("Unsupported interpolation mode");
        }
        if (i && !rage_time_after(point->time, *t)) {
            RAGE_ERROR("Times are not monotonicly increasing");
        }
        t = &point->time;
    }
    RAGE_OK
}

static rage_FrameSeries as_frameseries(
        rage_TupleDef const * const tuple_def, rage_TimeSeries const * points,
        uint32_t const sample_rate) {
    rage_FrameSeries fs;
    fs.len = points->len;
    fs.items = calloc(fs.len, sizeof(rage_FramePoint));
    for (uint32_t i = 0; i < points->len; i++) {
        rage_TimePoint const * p = &points->items[i];
        fs.items[i] = (rage_FramePoint) {
            .frame = rage_time_to_frame(&p->time, sample_rate),
            .value = rage_tuple_copy(tuple_def, p->value),
            .mode = p->mode
        };
    }
    return fs;
}

static void frameseries_free(
        rage_TupleDef const * const tuple_def, rage_FrameSeries * points) {
    for (uint32_t i = 0; i < points->len; i++) {
        rage_tuple_free(tuple_def, points->items[i].value);
    }
    points->len = 0;
    free(points->items);
}

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

static rage_InterpolationMode allowed_interpolators(
        rage_TupleDef const * const td) {
    rage_InterpolationMode m =
        RAGE_INTERPOLATION_CONST |
        RAGE_INTERPOLATION_LINEAR;
    for (unsigned i=0; i < td->len; i++) {
        switch (td->items[i].type->type) {
            case (RAGE_ATOM_FLOAT):
            case (RAGE_ATOM_INT):
                break;
            case (RAGE_ATOM_TIME):
            case (RAGE_ATOM_STRING):
            case (RAGE_ATOM_ENUM):
                m = RAGE_INTERPOLATION_CONST;
                break;
        }
    }
    return m;
}

rage_InitialisedInterpolator rage_interpolator_new(
        rage_TupleDef const * const type, rage_TimeSeries const * points,
        uint32_t const sample_rate) {
    rage_Error err = validate_time_series(points, allowed_interpolators(type));
    if (RAGE_FAILED(err)) {
        RAGE_FAIL(rage_InitialisedInterpolator, RAGE_FAILURE_VALUE(err));
    }
    rage_Interpolator * state = malloc(sizeof(rage_Interpolator));
    state->len = type->len;
    state->value = calloc(sizeof(rage_Atom), state->len);
    state->interpolators = calloc(sizeof(rage_AtomInterpolator), state->len);
    for (unsigned i=0; i < state->len; i++) {
        rage_AtomDef const * atom_def = type->items[i].type;
        switch (atom_def->type) {
            case (RAGE_ATOM_FLOAT):
                state->interpolators[i] = rage_float_interpolate;
                break;
            case (RAGE_ATOM_INT):
                state->interpolators[i] = rage_int_interpolate;
                break;
            case (RAGE_ATOM_TIME):
            case (RAGE_ATOM_STRING):
            case (RAGE_ATOM_ENUM):
                state->interpolators[i] = NULL;
        }
    }
    state->points = as_frameseries(type, points, sample_rate);
    RAGE_SUCCEED(rage_InitialisedInterpolator, state);
}

void rage_interpolator_free(
        rage_TupleDef const * const type, rage_Interpolator * const state) {
    frameseries_free(type, &state->points);
    free(state->value);
    free(state->interpolators);
    free(state);
}

rage_InterpolatedValue rage_interpolate(
        rage_Interpolator * const state, rage_FrameNo consumed) {
    rage_FramePoint const * start = NULL;
    rage_FramePoint const * end = NULL;
    unsigned i;
    state->pos += consumed;
    for (i = 0; i < state->points.len; i++) {
        if (state->points.items[i].frame > state->pos) {
            end = &(state->points.items[i]);
            break;
        }
        start = &(state->points.items[i]);
    }
    if (end == NULL) {
        return (rage_InterpolatedValue) {.value=start->value, .valid_for=UINT32_MAX};
    }
    if (start->mode == RAGE_INTERPOLATION_CONST) {
        return (rage_InterpolatedValue) {
            .value=start->value, .valid_for=end->frame - state->pos};
    } else {
        float const weight =
            (float) (state->pos - start->frame) /
            (float) (end->frame - start->frame);
        for (i = 0; i < state->len; i++) {
            state->interpolators[i](
                state->value + i, start->value + i, end->value + i, weight);
        }
        return (rage_InterpolatedValue) {.value=state->value, .valid_for=1};
    }
}
