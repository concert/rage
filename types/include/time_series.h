#pragma once

#include "time.h"
#include "atoms.h"
#include "macros.h"

// FIXME: This feels like it should be coming from interpolation somehow?
typedef enum {
    RAGE_INTERPOLATION_CONST,
    RAGE_INTERPOLATION_LINEAR
} rage_InterpolationMode;

typedef struct {
    rage_Time time;
    rage_Tuple value;
    rage_InterpolationMode mode;
    // rage_Tuple interpolation_args;
} rage_TimePoint;

typedef RAGE_ARRAY(rage_TimePoint) rage_TimeSeries;

rage_TimeSeries rage_time_series_new(rage_TupleDef const * const item_def);
void rage_time_series_free(rage_TimeSeries ts);
