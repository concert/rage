#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <stdatomic.h>
#include <semaphore.h>

#include "interpolation.h"
#include "countdown.h"
#include "queue.h"

#define RAGE_INTERPOLATORS_N RAGE_INTERPOLATION_LINEAR + 1

typedef uint32_t (*rage_AtomInterpolator)(
    rage_Atom * const target,
    rage_Atom const * const start_value, rage_Atom const * const end_value,
    const uint32_t frames_through, const uint32_t duration);

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

typedef struct {
    rage_QueueItem * qi;
    rage_Event * event;
    rage_FrameSeries fs;
    rage_Interpolator * interpolator;
} rage_FrameSeriesChangedInfo;

struct rage_Interpolator {
    rage_TupleDef const * type;
    uint32_t sample_rate;
    RAGE_ARRAY(rage_AtomInterpolator *) interpolators;
    sem_t change_sem;
    rage_FrameSeries * _Atomic points;
    rage_FrameNo valid_from;
    rage_FrameSeriesChangedInfo * _Atomic completed_change;
    rage_Queue * q;
    rage_Event * event;
    rage_QueueItem * qi;
    RAGE_ARRAY(rage_InterpolatedView) views;
};

struct rage_InterpolatedView {
    rage_Interpolator * interpolator;
    rage_FrameSeries * points;
    rage_InterpolatedValue value;
    rage_FrameNo pos;
};

static void rage_interpolatedview_init(
        rage_Interpolator * interpolator, rage_InterpolatedView * iv) {
    iv->interpolator = interpolator;
    iv->points = atomic_load_explicit(&interpolator->points, memory_order_relaxed);
    iv->value.value = calloc(sizeof(rage_Atom), interpolator->interpolators.len);
    iv->pos = 0;
}

static void rage_interpolatedview_destroy(rage_InterpolatedView * iv) {
    free(iv->value.value);
}

static rage_Error validate_time_series(
	rage_TimeSeries const * points, rage_InterpolationMode mode_limit) {
    if (points->len == 0) {
        return RAGE_ERROR("No points in time series");
    }
    rage_TimePoint const * point = &points->items[0];
    rage_Time const * t = &point->time;
    for (uint32_t i = 0; i < points->len; i++) {
        point = &points->items[i];
        if ((point->mode < RAGE_INTERPOLATION_CONST) || (point->mode > mode_limit)) {
            return RAGE_ERROR("Unsupported interpolation mode");
        }
        if (i && !rage_time_after(point->time, *t)) {
            return RAGE_ERROR("Times are not monotonicly increasing");
        }
        t = &point->time;
    }
    return RAGE_OK;
}

static void frameseries_destroy(
        rage_TupleDef const * const tuple_def, rage_FrameSeries * points) {
    rage_countdown_free(points->c);
    for (uint32_t i = 0; i < points->len; i++) {
        rage_tuple_free(tuple_def, points->items[i].value);
    }
    points->len = 0;
    free(points->items);
    points->items = NULL;
}

static void init_frameseries_points(
        rage_FrameSeries * const fs, rage_TupleDef const * const tuple_def,
        rage_TimeSeries const * const points, uint32_t const sample_rate) {
    RAGE_ARRAY_INIT(fs, points->len, i) {
        rage_TimePoint const * p = &points->items[i];
        fs->items[i] = (rage_FramePoint) {
            .frame = rage_time_to_frame(&p->time, sample_rate),
            .value = rage_tuple_copy(tuple_def, p->value),
            .mode = p->mode
        };
    }
}

static void frameseries_done_with(void * vp) {
    rage_FrameSeriesChangedInfo * fsi = vp;
    atomic_store_explicit(&fsi->interpolator->completed_change, fsi, memory_order_relaxed);
}

static void frameseries_change_event_free(void * vp) {
    rage_FrameSeriesChangedInfo * e = vp;
    frameseries_destroy(e->interpolator->type, &e->fs);
    free(e);
}

rage_EventType * rage_EventTimeSeriesChanged = "time series changed";

static rage_FrameSeriesChangedInfo * fsci_new(
        rage_Interpolator * state, rage_TimeSeries const * const ts,
        int const n_views) {
    rage_FrameSeriesChangedInfo * fsci = malloc(sizeof(rage_FrameSeriesChangedInfo));
    init_frameseries_points(&fsci->fs, state->type, ts, state->sample_rate);
    fsci->fs.c = rage_countdown_new(n_views, frameseries_done_with, fsci);
    fsci->event = rage_event_new(
        rage_EventTimeSeriesChanged, (void *) &fsci->fs, NULL, frameseries_change_event_free, fsci);
    fsci->qi = rage_queue_item_new(fsci->event);
    fsci->interpolator = state;
    return fsci;
}

static uint32_t rage_const_interpolate_fwd(
        rage_Atom * const target,
        rage_Atom const * const start_value, rage_Atom const * const end_value,
        const uint32_t frames_through, const uint32_t duration) {
    *target = *start_value;
    return duration - frames_through;
}

static uint32_t rage_time_interpolate(
        rage_Atom * const target,
        rage_Atom const * const start_value, rage_Atom const * const end_value,
        const uint32_t frames_through, const uint32_t duration) {
    target->frame_no = start_value->frame_no + frames_through;
    return duration - frames_through;
}

#define RAGE_NUMERIC_INTERPOLATE(type, member) \
    static uint32_t rage_##type##_linear_interpolate( \
            rage_Atom * const target, \
            rage_Atom const * const start_value, rage_Atom const * const end_value, \
            const uint32_t frames_through, const uint32_t duration) { \
        float const weight = (float) frames_through / (float) duration; \
        target->member = (start_value->member * (1 - weight)) + (end_value->member * weight); \
        return 1; \
    }

RAGE_NUMERIC_INTERPOLATE(float, f)
RAGE_NUMERIC_INTERPOLATE(int, i)

#undef RAGE_NUMERIC_INTERPOLATE

static rage_AtomInterpolator const_interpolators[RAGE_INTERPOLATORS_N] = {
    rage_const_interpolate_fwd, NULL
};

static rage_AtomInterpolator int_interpolators[RAGE_INTERPOLATORS_N] = {
    rage_const_interpolate_fwd, rage_int_linear_interpolate
};

static rage_AtomInterpolator float_interpolators[RAGE_INTERPOLATORS_N] = {
    rage_const_interpolate_fwd, rage_float_linear_interpolate
};

static rage_AtomInterpolator time_interpolators[RAGE_INTERPOLATORS_N] = {
    rage_time_interpolate, NULL
};

rage_InterpolationMode rage_interpolation_limit(
        rage_TupleDef const * const td) {
    rage_InterpolationMode m = RAGE_INTERPOLATION_LINEAR;
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
        uint32_t const sample_rate, uint8_t const n_views,
        rage_Queue * evt_queue) {
    rage_Error err = validate_time_series(points, rage_interpolation_limit(type));
    if (RAGE_FAILED(err)) {
        return RAGE_FAILURE_CAST(rage_InitialisedInterpolator, err);
    }
    rage_Interpolator * state = malloc(sizeof(rage_Interpolator));
    state->type = type;
    state->sample_rate = sample_rate;
    RAGE_ARRAY_INIT(&state->interpolators, type->len, i) {
        rage_AtomDef const * atom_def = type->items[i].type;
        switch (atom_def->type) {
            case (RAGE_ATOM_FLOAT):
                state->interpolators.items[i] = float_interpolators;
                break;
            case (RAGE_ATOM_INT):
                state->interpolators.items[i] = int_interpolators;
                break;
            case (RAGE_ATOM_TIME):
                state->interpolators.items[i] = time_interpolators;
                break;
            case (RAGE_ATOM_STRING):
            case (RAGE_ATOM_ENUM):
                state->interpolators.items[i] = const_interpolators;
        }
    }
    sem_init(&state->change_sem, 0, 1);
    state->q = evt_queue;
    rage_FrameSeriesChangedInfo * fsci = fsci_new(state, points, n_views);
    state->event = fsci->event;
    state->qi = fsci->qi;
    atomic_init(&state->completed_change, NULL);
    atomic_init(&state->points, &fsci->fs);
    RAGE_ARRAY_INIT(&state->views, n_views, i) {
        rage_interpolatedview_init(state, &state->views.items[i]);
    }
    return RAGE_SUCCESS(rage_InitialisedInterpolator, state);
}

void rage_interpolator_free(
        rage_TupleDef const * const type, rage_Interpolator * const state) {
    for (unsigned i=0; i < state->views.len; i++) {
        rage_interpolatedview_destroy(&state->views.items[i]);
    }
    free(state->views.items);
    free(state->interpolators.items);
    sem_destroy(&state->change_sem);
    rage_event_free(state->event);
    rage_queue_item_free(state->qi);
    free(state);
}

rage_InterpolatedValue const * rage_interpolated_view_value(
        rage_InterpolatedView * const view) {
    rage_FramePoint const * start = NULL;
    rage_FramePoint const * end = NULL;
    unsigned i;
    uint32_t duration;
    for (i = 0; i < view->points->len; i++) {
        if (view->points->items[i].frame > view->pos) {
            end = &(view->points->items[i]);
            break;
        }
        start = &(view->points->items[i]);
    }
    if (start == NULL) {
        view->value.valid_for = view->points->items[0].frame - view->pos;
        for (i = 0; i < view->interpolator->interpolators.len; i++) {
            view->value.value[i] = end->value[i];
        }
    } else if (end == NULL) {
        view->value.valid_for = duration = UINT32_MAX;
        for (i = 0; i < view->interpolator->interpolators.len; i++) {
            view->interpolator->interpolators.items[i][RAGE_INTERPOLATION_CONST](
                view->value.value + i, start->value + i, NULL,
                view->pos - start->frame, duration);
        }
    } else {
        duration = end->frame - start->frame;
        for (i = 0; i < view->interpolator->interpolators.len; i++) {
            view->value.valid_for = view->interpolator->interpolators.items[i][start->mode](
                view->value.value + i, start->value + i, end->value + i,
                view->pos - start->frame, duration);
        }
    }
    // Take into account that timeseries might have changed in the future
    rage_FrameSeries * active_pts = atomic_load_explicit(
        &view->interpolator->points, memory_order_consume);
    if (active_pts != view->points) {
        uint32_t next_view_valid_from = view->interpolator->valid_from - view->pos;
        view->value.valid_for = (view->value.valid_for < next_view_valid_from) ?
            view->value.valid_for : next_view_valid_from;
    }
    return &view->value;
}

rage_FrameNo rage_interpolated_view_get_pos(
        rage_InterpolatedView const * const view) {
    return view->pos;
}

void rage_interpolated_view_seek(
        rage_InterpolatedView * const view, rage_FrameNo frame_no) {
    view->pos = frame_no;
    rage_FrameSeries * active_pts = atomic_load_explicit(
        &view->interpolator->points, memory_order_consume);
    if (
            active_pts != view->points &&
            view->pos >= view->interpolator->valid_from) {
        rage_countdown_add(view->points->c, -1);
        view->points = active_pts;
    }
    rage_FrameSeriesChangedInfo * const fsci = atomic_exchange(
        &view->interpolator->completed_change, NULL);
    if (fsci != NULL) {
        if (!rage_queue_put_nonblock(view->interpolator->q, fsci->qi)) {
            atomic_store(&view->interpolator->completed_change, fsci);
        }
    }
}

void rage_interpolated_view_advance(
        rage_InterpolatedView * const view, rage_FrameNo frames) {
    rage_interpolated_view_seek(view, view->pos + frames);
}

rage_NewEventId rage_interpolator_change_timeseries(
        rage_Interpolator * const state, rage_TimeSeries const ts,
        rage_FrameNo change_at) {
    rage_Error const val_err = validate_time_series(&ts, rage_interpolation_limit(state->type));
    if (RAGE_FAILED(val_err)) {
        return RAGE_FAILURE_CAST(rage_NewEventId, val_err);
    }
    rage_FrameSeriesChangedInfo * fsci = fsci_new(state, &ts, state->views.len);
    sem_wait(&state->change_sem);
    rage_EventId const eid = rage_event_id(state->event);
    state->event = fsci->event;
    state->qi = fsci->qi;
    state->valid_from = change_at;
    atomic_store_explicit(&state->points, &fsci->fs, memory_order_release);
    return RAGE_SUCCESS(rage_NewEventId, eid);
}

rage_InterpolatedView * rage_interpolator_get_view(rage_Interpolator * state, uint8_t idx) {
    return &state->views.items[idx];
}
