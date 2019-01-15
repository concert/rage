#include "element_helpers.h"

uint8_t view_count_for_type(rage_ElementType const * const type) {
    uint8_t n_views = 1;
    if (type->prep != NULL) {
        n_views++;
    }
    if (type->clean != NULL) {
        n_views++;
    }
    return n_views;
}

InterpolatorsForResult interpolators_for(
        uint32_t sample_rate,
        rage_InstanceSpecControls const * control_spec,
        rage_TimeSeries const * const control_values, uint8_t const n_views) {
    uint32_t const n_controls = control_spec->len;
    rage_Interpolator ** new_interpolators = calloc(
        n_controls, sizeof(rage_Interpolator *));
    if (new_interpolators == NULL) {
        return RAGE_FAILURE(
            InterpolatorsForResult,
            "Unable to allocate memory for new interpolators");
    }
    for (uint32_t i = 0; i < n_controls; i++) {
        rage_InitialisedInterpolator ii = rage_interpolator_new(
            &control_spec->items[i], &control_values[i], sample_rate,
            n_views, NULL);
        if (RAGE_FAILED(ii)) {
            if (i) {
                do {
                    i--;
                    rage_interpolator_free(
                        &control_spec->items[i], new_interpolators[i]);
                } while (i > 0);
            }
            free(new_interpolators);
            return RAGE_FAILURE_CAST(InterpolatorsForResult, ii);
            break;
        } else {
            new_interpolators[i] = RAGE_SUCCESS_VALUE(ii);
        }
    }
    return RAGE_SUCCESS(InterpolatorsForResult, new_interpolators);
}
