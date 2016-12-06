#pragma once

#include "time.h"
#include "atoms.h"
#include "error.h"
#include "time_series.h"

typedef uint64_t rage_FrameNo;

typedef struct rage_Interpolator rage_Interpolator;

typedef struct {
    rage_Atom * value;
    rage_FrameNo valid_for;
} rage_InterpolatedValue;

// Should this return a pointer?
rage_InterpolatedValue rage_interpolate(
    rage_Interpolator * state, rage_FrameNo consumed);

// FIXME: different headers?

typedef RAGE_OR_ERROR(rage_Interpolator *) rage_InitialisedInterpolator;

rage_InitialisedInterpolator rage_interpolator_new(
    rage_TupleDef const * const type, rage_TimeSeries const * points,
    uint32_t const sample_rate);
void rage_interpolator_free(
    rage_TupleDef const * const type, rage_Interpolator * state);

void rage_interpolator_seek(rage_Interpolator * state, rage_FrameNo pos);
