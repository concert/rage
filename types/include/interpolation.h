#pragma once

#include "chronology.h"
#include "atoms.h"
#include "error.h"
#include "time_series.h"
#include "finaliser.h"
#include "event.h"
#include "queue.h"

typedef struct rage_Interpolator rage_Interpolator;
typedef struct rage_InterpolatedView rage_InterpolatedView;
typedef RAGE_ARRAY(rage_InterpolatedView) rage_InterpolatedViews;
typedef RAGE_OR_ERROR(rage_EventId) rage_NewEvent;

extern rage_EventType * rage_EventTimeSeriesChanged;

typedef struct {
    rage_Atom * value;
    uint32_t valid_for;
} rage_InterpolatedValue;

rage_InterpolatedValue const * rage_interpolated_view_value(
    rage_InterpolatedView * const view);

rage_FrameNo rage_interpolated_view_get_pos(
    rage_InterpolatedView const * const view);
void rage_interpolated_view_advance(
    rage_InterpolatedView * view, rage_FrameNo n_frames);

// FIXME: different headers?

typedef RAGE_OR_ERROR(rage_Interpolator *) rage_InitialisedInterpolator;

rage_InitialisedInterpolator rage_interpolator_new(
    rage_TupleDef const * const type, rage_TimeSeries const * points,
    uint32_t const sample_rate, uint8_t const n_views, rage_Queue * evt_queue);
void rage_interpolator_free(
    rage_TupleDef const * const type, rage_Interpolator * state);
void rage_interpolated_view_seek(
    rage_InterpolatedView * view, rage_FrameNo frame_no);
rage_NewEvent rage_interpolator_change_timeseries(
    rage_Interpolator * const state, rage_TimeSeries const * const ts,
    rage_FrameNo change_at);
rage_InterpolatedView * rage_interpolator_get_view(rage_Interpolator * state, uint8_t idx);

rage_InterpolationMode rage_interpolation_limit(rage_TupleDef const * const td);
