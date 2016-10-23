#pragma once

#include "time.h"
#include "atoms.h"
#include "macros.h"

typedef enum {
    RAGE_INTERPOLATION_CONST,
    RAGE_INTERPOLATION_LINEAR
} rage_InterpolationMode;

typedef struct {
    rage_Time time;
    rage_Tuple value;
    rage_InterpolationMode mode;
    // rage_Tuple args;
} rage_TimePoint;

typedef RAGE_ARRAY(rage_TimePoint) rage_TimeSeries;

typedef struct {
    uint32_t len;
    rage_Tuple value;
} rage_Interpolator_;
typedef rage_Interpolator_ * rage_Interpolator;

rage_Interpolator rage_interpolator_new(rage_TupleDef type);
void rage_interpolator_free(rage_Interpolator state);
rage_Tuple rage_interpolate(rage_Interpolator state, rage_Time time, rage_TimeSeries points);
