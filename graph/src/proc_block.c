#include "proc_block.h"
#include "jack_bindings.h"
#include "srt.h"
#include "element_helpers.h"
#include <stdlib.h>

struct rage_ProcBlock {
    rage_Countdown * countdown;
    rage_JackBinding * jack_binding;
    rage_SupportConvoy * convoy;
    uint32_t sample_rate;
};

typedef struct {
    rage_InterpolatedView ** prep;
    rage_InterpolatedView ** clean;
    rage_InterpolatedView ** rt;
} rage_ProcBlockViews;

struct rage_Harness {
    rage_JackHarness * jack_harness;
    rage_SupportTruck * truck;
    rage_Interpolator ** interpolators;
    rage_ProcBlockViews views;
};

rage_NewProcBlock rage_proc_block_new(
        uint32_t sample_rate, rage_TransportState transp_state) {
    rage_Countdown * countdown = rage_countdown_new(0);
    rage_NewJackBinding njb = rage_jack_binding_new(countdown, sample_rate);
    if (RAGE_FAILED(njb)) {
        rage_countdown_free(countdown);
        return RAGE_FAILURE_CAST(rage_NewProcBlock, njb);
    } else {
        rage_ProcBlock * pb = malloc(sizeof(rage_ProcBlock));
        pb->jack_binding = RAGE_SUCCESS_VALUE(njb);
        pb->sample_rate = sample_rate;
        // FIXME: FIXED PERIOD SIZE!!!!!
        // The success value should probably be some kind of handy struct
        pb->convoy = rage_support_convoy_new(1024, countdown, transp_state);
        pb->countdown = countdown;
        return RAGE_SUCCESS(rage_NewProcBlock, pb);
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
    return rage_support_convoy_stop(pb->convoy);
}

typedef RAGE_OR_ERROR(rage_Interpolator **) InterpolatorsForResult;

static InterpolatorsForResult interpolators_for(
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
            n_views);
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

static rage_ProcBlockViews rage_proc_block_initialise_views(
        rage_ConcreteElementType * cet, rage_Harness * harness) {
    rage_ProcBlockViews views;
    views.rt = calloc(
        cet->controls.len, sizeof(rage_InterpolatedView *));
    if (cet->type->prep == NULL) {
        views.prep = NULL;
    } else {
        views.prep = calloc(
            cet->controls.len, sizeof(rage_InterpolatedView *));
    }
    if (cet->type->clean == NULL) {
        views.clean = NULL;
    } else {
        views.clean = calloc(
            cet->controls.len, sizeof(rage_InterpolatedView *));
    }
    for (uint32_t i = 0; i < cet->controls.len; i++) {
        uint8_t view_idx = 0;
        views.rt[i] = rage_interpolator_get_view(
            harness->interpolators[i], view_idx);
        if (views.prep != NULL) {
            views.prep[i] = rage_interpolator_get_view(
                harness->interpolators[i], ++view_idx);
        }
        if (views.clean != NULL) {
            views.clean[i] = rage_interpolator_get_view(
                harness->interpolators[i], ++view_idx);
        }
    }
    return views;
}

rage_MountResult rage_proc_block_mount(
        rage_ProcBlock * pb, rage_Element * elem,
        rage_TimeSeries const * controls, char const * name) {
    rage_Harness * harness = malloc(sizeof(rage_Harness));
    uint8_t n_views = view_count_for_type(elem->cet->type);
    // FIXME: Error handling
    harness->interpolators = RAGE_SUCCESS_VALUE(interpolators_for(
        pb->sample_rate, &elem->cet->controls, controls, n_views));
    harness->views = rage_proc_block_initialise_views(elem->cet, harness);
    // FIXME: could be more efficient, and not add this if not required
    harness->truck = rage_support_convoy_mount(
        pb->convoy, elem, harness->views.prep, harness->views.clean);
    harness->jack_harness = rage_jack_binding_mount(
        pb->jack_binding, elem, harness->views.rt, name);
    return RAGE_SUCCESS(rage_MountResult, harness);
}

void rage_proc_block_unmount(rage_Harness * harness) {
    // This could handle the aligning of unmounts and the thing being unmounted
    // from (reducing backrefs)
    rage_jack_binding_unmount(harness->jack_harness);
    rage_support_convoy_unmount(harness->truck);
    free(harness->views.prep);
    free(harness->views.clean);
    free(harness->views.rt);
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

void rage_proc_block_set_transport_state(rage_ProcBlock * pb, rage_TransportState state) {
    // We call SRT either side of transport changes as starting may require
    // preparation, and stopping may require cleanup, but they need to happen at
    // different times wrt the RT change
    rage_support_convoy_transport_state_changing(pb->convoy, state);
    rage_jack_binding_set_transport_state(pb->jack_binding, state);
    rage_support_convoy_transport_state_changed(pb->convoy, state);
}

rage_Error rage_proc_block_transport_seek(rage_ProcBlock * pb, rage_FrameNo target) {
    rage_Error e = rage_support_convoy_transport_seek(pb->convoy, target);
    if (RAGE_FAILED(e))
        return e;
    return rage_jack_binding_transport_seek(pb->jack_binding, target);
}
