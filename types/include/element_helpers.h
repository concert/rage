#pragma once
#include "element.h"
#include "queue.h"

typedef RAGE_OR_ERROR(rage_Interpolator **) InterpolatorsForResult;

uint8_t view_count_for_type(rage_ElementType const * const type);
InterpolatorsForResult interpolators_for(
        uint32_t sample_rate, rage_Queue * q,
        rage_InstanceSpecControls const * control_spec,
        rage_TimeSeries const * const control_values, uint8_t const n_views);
