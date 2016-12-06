#pragma once

#include "time.h"
#include "atoms.h"
#include "macros.h"

// FIXME: This feels like it should be coming from interpolation somehow?
typedef enum {
    RAGE_INTERPOLATION_CONST = 0x01,
    RAGE_INTERPOLATION_LINEAR = 0x02
} rage_InterpolationMode;

typedef struct {
    rage_Time time;
    rage_Atom * value;
    rage_InterpolationMode mode;
    // rage_Atom * interpolation_args;
} rage_TimePoint;

typedef RAGE_ARRAY(rage_TimePoint) rage_TimeSeries;

rage_TimeSeries rage_time_series_new(rage_TupleDef const * const item_def);
void rage_time_series_free(rage_TimeSeries ts);
