#include "proc_block.h"
#include "jack_bindings.h"
#include "srt.h"
#include <stdlib.h>

struct rage_ProcBlock {
    rage_Countdown * countdown;
    rage_JackBinding * jack_binding;
    rage_SupportConvoy * convoy;
};

struct rage_Harness {
    rage_JackHarness * jack_harness;
    rage_SupportTruck * truck;
    rage_Interpolator ** interpolators;
};

rage_NewProcBlock rage_proc_block_new() {
    rage_Countdown * countdown = rage_countdown_new(0);
    rage_NewJackBinding njb = rage_jack_binding_new(countdown);
    if (RAGE_FAILED(njb)) {
        rage_countdown_free(countdown);
        RAGE_FAIL(rage_NewProcBlock, RAGE_FAILURE_VALUE(njb));
    } else {
        rage_ProcBlock * pb = malloc(sizeof(rage_ProcBlock));
        pb->jack_binding = RAGE_SUCCESS_VALUE(njb);
        // FIXME: FIXED PERIOD SIZE!!!!!
        // The success value should probably be some kind of handy struct
        pb->convoy = rage_support_convoy_new(1024, countdown);
        pb->countdown = countdown;
        RAGE_SUCCEED(rage_NewProcBlock, pb);
    }
}

void rage_proc_block_free(rage_ProcBlock * pb) {
    rage_countdown_free(pb->countdown);
    rage_jack_binding_free(pb->jack_binding);
    rage_support_convoy_free(pb->convoy);
    free(pb);
}

rage_Error rage_proc_block_start(rage_ProcBlock * pb) {
    rage_Error rv = rage_support_convoy_start(pb->convoy);
    if (RAGE_FAILED(rv)) {
        return rv;
    }
    return rage_jack_binding_start(pb->jack_binding);
}

rage_Error rage_proc_block_stop(rage_ProcBlock * pb) {
    rage_Error rv = rage_jack_binding_stop(pb->jack_binding);
    if (RAGE_FAILED(rv)) {
        return rv;
    }
    return rage_support_convoy_start(pb->convoy);
}

typedef RAGE_OR_ERROR(rage_Interpolator **) InterpolatorsForResult;

static InterpolatorsForResult interpolators_for(
        rage_ProcessRequirementsControls const * control_requirements,
        rage_TimeSeries * const control_values, uint8_t const n_views) {
    uint32_t const n_controls = control_requirements->len;
    rage_Interpolator ** new_interpolators = calloc(
        n_controls, sizeof(rage_Interpolator *));
    if (new_interpolators == NULL) {
        RAGE_FAIL(
            InterpolatorsForResult,
            "Unable to allocate memory for new interpolators")
    }
    for (uint32_t i = 0; i < n_controls; i++) {
        // FIXME: const sample rate
        rage_InitialisedInterpolator ii = rage_interpolator_new(
            &control_requirements->items[i], &control_values[i], 44100,
            n_views);
        if (RAGE_FAILED(ii)) {
            if (i) {
                do {
                    i--;
                    rage_interpolator_free(
                        &control_requirements->items[i], new_interpolators[i]);
                } while (i > 0);
            }
            free(new_interpolators);
            RAGE_FAIL(InterpolatorsForResult, RAGE_FAILURE_VALUE(ii));
            break;
        } else {
            new_interpolators[i] = RAGE_SUCCESS_VALUE(ii);
        }
    }
    RAGE_SUCCEED(InterpolatorsForResult, new_interpolators)
}

rage_MountResult rage_proc_block_mount(
        rage_ProcBlock * pb, rage_Element * elem, rage_TimeSeries * controls,
        char const * name) {
    rage_Harness * harness = malloc(sizeof(rage_Harness));
    // FIXME: Error handling
    uint8_t n_views = 1;
    if (elem->type->prep != NULL) {
        n_views++;
    }
    if (elem->type->clean != NULL) {
        n_views++;
    }
    harness->interpolators = RAGE_SUCCESS_VALUE(interpolators_for(
        &elem->controls, controls, n_views));
    rage_InterpolatedView ** rt_views = calloc(
        elem->controls.len, sizeof(rage_InterpolatedView *));
    rage_InterpolatedView ** prep_views, ** clean_views;
    if (elem->type->prep == NULL) {
        prep_views = NULL;
    } else {
        prep_views = calloc(
            elem->controls.len, sizeof(rage_InterpolatedView *));
    }
    if (elem->type->clean == NULL) {
        clean_views = NULL;
    } else {
        clean_views = calloc(
            elem->controls.len, sizeof(rage_InterpolatedView *));
    }
    for (uint32_t i = 0; i < elem->controls.len; i++) {
        uint8_t view_idx = 0;
        rt_views[i] = rage_interpolator_get_view(
            harness->interpolators[i], view_idx);
        if (prep_views != NULL) {
            prep_views[i] = rage_interpolator_get_view(
                harness->interpolators[i], ++view_idx);
        }
        if (clean_views != NULL) {
            clean_views[i] = rage_interpolator_get_view(
                harness->interpolators[i], ++view_idx);
        }
    }
    harness->truck = rage_support_convoy_mount(
        pb->convoy, elem, prep_views, clean_views);
    harness->jack_harness = rage_jack_binding_mount(
        pb->jack_binding, elem, rt_views, name);
    RAGE_SUCCEED(rage_MountResult, harness);
}

void rage_proc_block_unmount(rage_Harness * harness) {
    // This could handle the aligning of unmounts and the thing being unmounted
    // from (reducing backrefs)
    rage_jack_binding_unmount(harness->jack_harness);
    // FIXME: Soft RT NOT HANDLED AT ALL
}

rage_Finaliser * rage_harness_set_time_series(
        rage_Harness * const harness,
        uint32_t const series_idx,
        rage_TimeSeries const * const new_controls) {
    // FIXME: fixed offset (should be derived from sample rate and period size at least)
    rage_FrameNo const offset = 4096;
    rage_InterpolatedView * first_rt_view = rage_interpolator_get_view(
        harness->interpolators[series_idx], 0);
    rage_FrameNo const change_at = rage_interpolated_view_get_pos(first_rt_view) + offset;
    return rage_interpolator_change_timeseries(harness->interpolators[series_idx], new_controls, change_at);
}
