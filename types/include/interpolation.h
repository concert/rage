#pragma once

#include "time.h"
#include "atoms.h"
#include "error.h"
#include "time_series.h"

typedef struct rage_Interpolator rage_Interpolator;

typedef enum {
    RAGE_TRANSPORT_STOPPED,
    RAGE_TRANSPORT_ROLLING
} rage_TransportState;

typedef struct {
    rage_Atom * value;
    rage_FrameNo valid_for;
} rage_InterpolatedValue;

rage_TransportState rage_interpolator_get_transport_state(
    rage_Interpolator * state);
// Should this return a pointer?
rage_InterpolatedValue rage_interpolate(
    rage_Interpolator * state, rage_FrameNo consumed);

// FIXME: different headers?

typedef RAGE_OR_ERROR(rage_Interpolator *) rage_InitialisedInterpolator;

rage_InitialisedInterpolator rage_interpolator_new(
    rage_TupleDef const * const type, rage_TimeSeries const * points,
    uint32_t const sample_rate, rage_TransportState transport);
void rage_interpolator_free(
    rage_TupleDef const * const type, rage_Interpolator * state);

void rage_interpolator_seek(rage_Interpolator * state, rage_FrameNo pos);
void rage_interpolator_set_transport_state(
    rage_Interpolator * state, rage_TransportState transport);
