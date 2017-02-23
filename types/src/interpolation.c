#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <stdatomic.h>
#include <semaphore.h>

#include "interpolation.h"
#include "countdown.h"

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

_Static_assert(
    ATOMIC_POINTER_LOCK_FREE,
    "Atomic pointers not lock free on this platform");

typedef struct {
    rage_Countdown * c;
    RAGE_ARRAY(rage_FramePoint);
} rage_FrameSeries;

struct rage_Interpolator {
    rage_TupleDef const * type;
    uint32_t sample_rate;
    RAGE_ARRAY(rage_AtomInterpolator) interpolators;
    sem_t change_sem;
    rage_FrameSeries * _Atomic points;
    rage_FrameNo valid_from;
    RAGE_ARRAY(rage_InterpolatedView) views;
};

struct rage_InterpolatedView {
    rage_Interpolator * interpolator;
    rage_FrameSeries * points;
    rage_InterpolatedValue value;
    rage_Atom * val;
    rage_FrameNo pos;
};

static void rage_interpolatedview_init(
        rage_Interpolator * interpolator, rage_InterpolatedView * iv) {
    iv->interpolator = interpolator;
    iv->points = atomic_load_explicit(&interpolator->points, memory_order_relaxed);
    iv->val = calloc(sizeof(rage_Atom), interpolator->interpolators.len);
    iv->pos = 0;
}

static void rage_interpolatedview_destroy(rage_InterpolatedView * iv) {
    rage_countdown_add(iv->points->c, -1);
    free(iv->val);
}

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

static rage_FrameSeries * as_frameseries(
        rage_TupleDef const * const tuple_def, rage_TimeSeries const * points,
        uint32_t const sample_rate, uint8_t const awaiting_completion) {
    rage_FrameSeries * fs = malloc(sizeof(rage_FrameSeries));
    fs->c = rage_countdown_new(awaiting_completion);
    RAGE_ARRAY_INIT(fs, points->len, i) {
        rage_TimePoint const * p = &points->items[i];
        fs->items[i] = (rage_FramePoint) {
            .frame = rage_time_to_frame(&p->time, sample_rate),
            .value = rage_tuple_copy(tuple_def, p->value),
            .mode = p->mode
        };
    }
    return fs;
}

static void frameseries_free(
        rage_TupleDef const * const tuple_def, rage_FrameSeries * points) {
    rage_countdown_free(points->c);
    for (uint32_t i = 0; i < points->len; i++) {
        rage_tuple_free(tuple_def, points->items[i].value);
    }
    points->len = 0;
    free(points->items);
    free(points);
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
        uint32_t const sample_rate, uint8_t const n_views) {
    rage_Error err = validate_time_series(points, allowed_interpolators(type));
    if (RAGE_FAILED(err)) {
        RAGE_FAIL(rage_InitialisedInterpolator, RAGE_FAILURE_VALUE(err));
    }
    rage_Interpolator * state = malloc(sizeof(rage_Interpolator));
    sem_init(&state->change_sem, 0, 1);
    state->type = type;
    state->sample_rate = sample_rate;
    atomic_init(&state->points, as_frameseries(type, points, sample_rate, n_views));
    RAGE_ARRAY_INIT(&state->interpolators, type->len, i) {
        rage_AtomDef const * atom_def = type->items[i].type;
        switch (atom_def->type) {
            case (RAGE_ATOM_FLOAT):
                state->interpolators.items[i] = rage_float_interpolate;
                break;
            case (RAGE_ATOM_INT):
                state->interpolators.items[i] = rage_int_interpolate;
                break;
            case (RAGE_ATOM_TIME):
            case (RAGE_ATOM_STRING):
            case (RAGE_ATOM_ENUM):
                state->interpolators.items[i] = NULL;
        }
    }
    RAGE_ARRAY_INIT(&state->views, n_views, i) {
        rage_interpolatedview_init(state, &state->views.items[i]);
    }
    RAGE_SUCCEED(rage_InitialisedInterpolator, state);
}

void rage_interpolator_free(
        rage_TupleDef const * const type, rage_Interpolator * const state) {
    for (unsigned i=0; i < state->views.len; i++) {
        rage_interpolatedview_destroy(&state->views.items[i]);
    }
    free(state->views.items);
    frameseries_free(
        type, atomic_load_explicit(&state->points, memory_order_relaxed));
    free(state->interpolators.items);
    sem_destroy(&state->change_sem);
    free(state);
}

rage_InterpolatedValue const * rage_interpolated_view_value(
        rage_InterpolatedView * const view) {
    rage_FramePoint const * start = NULL;
    rage_FramePoint const * end = NULL;
    unsigned i;
    for (i = 0; i < view->points->len; i++) {
        if (view->points->items[i].frame > view->pos) {
            end = &(view->points->items[i]);
            break;
        }
        start = &(view->points->items[i]);
    }
    if (end == NULL) {
        view->value.value = start->value;
        view->value.valid_for = UINT32_MAX;
    } else {
        if (start->mode == RAGE_INTERPOLATION_CONST) {
            view->value.value = start->value;
            view->value.valid_for = end->frame - view->pos;
        } else {
            float const weight =
                (float) (view->pos - start->frame) /
                (float) (end->frame - start->frame);
            for (i = 0; i < view->interpolator->interpolators.len; i++) {
                view->interpolator->interpolators.items[i](
                    view->val + i, start->value + i, end->value + i, weight);
            }
            view->value.value = view->val;
            view->value.valid_for = 1;
        }
    }
    return &view->value;
}

rage_FrameNo rage_interpolated_view_get_pos(
        rage_InterpolatedView const * const view) {
    return view->pos;
}

void rage_interpolated_view_seek(
        rage_InterpolatedView * const view, rage_FrameNo frame_no) {
    rage_FrameSeries * active_pts = atomic_load_explicit(
        &view->interpolator->points, memory_order_consume);
    view->pos = frame_no;
    if (
            active_pts != view->points &&
            view->pos >= view->interpolator->valid_from) {
        rage_countdown_add(view->points->c, -1);
        view->points = active_pts;
    }
}

void rage_interpolated_view_advance(
        rage_InterpolatedView * const view, rage_FrameNo frames) {
    rage_interpolated_view_seek(view, view->pos + frames);
}

typedef struct {
    sem_t * change_sem;
    rage_TupleDef const * type;
    rage_FrameSeries * fs;
} rage_TimeSeriesChange;

static void rage_finalise_timeseries_change(void * p) {
    rage_TimeSeriesChange * tsc = p;
    rage_countdown_timed_wait(tsc->fs->c, 500); // FIXME: ERROR HANDLING!
    sem_post(tsc->change_sem);
    frameseries_free(tsc->type, tsc->fs);
}

rage_Finaliser * rage_interpolator_change_timeseries(
        rage_Interpolator * const state, rage_TimeSeries const * const ts,
        rage_FrameNo change_at) {
    rage_FrameSeries * fs = as_frameseries(
        state->type, ts, state->sample_rate, state->views.len);
    rage_TimeSeriesChange * tsc = malloc(sizeof(rage_TimeSeriesChange));
    tsc->change_sem = &state->change_sem;
    tsc->type = state->type;
    sem_wait(&state->change_sem);
    tsc->fs = atomic_load_explicit(&state->points, memory_order_relaxed);
    state->valid_from = change_at;
    atomic_store_explicit(&state->points, fs, memory_order_release);
    return rage_finaliser_new(rage_finalise_timeseries_change, (void *) tsc);
}

rage_InterpolatedView * rage_interpolator_get_view(rage_Interpolator * state, uint8_t idx) {
    return &state->views.items[idx];
}
