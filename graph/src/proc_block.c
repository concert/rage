#include "proc_block.h"
#include "srt.h"
#include "element_helpers.h"
#include "rtcrit.h"
#include "binding_interface.h"
#include <stdlib.h>


typedef struct {
    rage_InterpolatedView ** prep;
    rage_InterpolatedView ** clean;
    rage_InterpolatedView ** rt;
} rage_ProcBlockViews;

struct rage_Harness {
    rage_Element * elem;
    // FIXME: Don't actually need a ports per elem (but 1 of the largest size)?
    union {
        rage_Ports;
        rage_Ports ports;
    };
    rage_SupportTruck * truck;
    rage_Interpolator ** interpolators;
    rage_ProcBlockViews views;
    rage_ProcBlock * pb;
};

typedef struct {
    uint32_t * in_buffer_allocs;
    uint32_t * out_buffer_allocs;
    rage_Harness * harness;
} rage_ProcStep;

typedef struct {
    rage_TransportState transp;
    void ** all_buffers;
    RAGE_ARRAY(rage_ProcStep) steps;
} rage_RtBits;

typedef struct rage_PBConnection {
    rage_Harness * source_harness;
    uint32_t source_idx;
    rage_Harness * sink_harness;
    uint32_t sink_idx;
    struct rage_PBConnection * next;
} rage_PBConnection;

struct rage_ProcBlock {
    uint32_t sample_rate;
    uint32_t n_inputs;
    uint32_t n_outputs;
    rage_Countdown * rolling_countdown;
    rage_SupportConvoy * convoy;
    rage_RtCrit * syncy;
    rage_PBConnection * cons;
};

rage_ProcBlock * rage_proc_block_new(
        uint32_t sample_rate, rage_TransportState transp_state) {
    rage_Countdown * countdown = rage_countdown_new(0);
    rage_ProcBlock * pb = malloc(sizeof(rage_ProcBlock));
    pb->cons = NULL;
    pb->sample_rate = sample_rate;
    // FIXME: hard coded mono
    pb->n_inputs = pb->n_outputs = 1;
    // FIXME: FIXED PERIOD SIZE!!!!!
    // The success value should probably be some kind of handy struct
    pb->rolling_countdown = countdown;
    pb->convoy = rage_support_convoy_new(1024, countdown, transp_state);
    rage_RtBits * rtb = malloc(sizeof(rage_RtBits));
    rtb->transp = transp_state;
    // FIXME: fixed all_buffers thing
    rtb->all_buffers = calloc(2, sizeof(void *));
    rtb->steps.len = 0;
    rtb->steps.items = NULL;
    pb->syncy = rage_rt_crit_new(rtb);
    return pb;
}

void rage_proc_block_free(rage_ProcBlock * pb) {
    rage_countdown_free(pb->rolling_countdown);
    rage_support_convoy_free(pb->convoy);
    rage_RtBits * rtb = rage_rt_crit_free(pb->syncy);
    free(rtb->all_buffers);
    // FIXME: free steps
    free(pb);
}

rage_Error rage_proc_block_start(rage_ProcBlock * pb) {
    return rage_support_convoy_start(pb->convoy);
}

rage_Error rage_proc_block_stop(rage_ProcBlock * pb) {
    return rage_support_convoy_stop(pb->convoy);
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
        rage_TimeSeries const * controls) {
    rage_Harness * harness = malloc(sizeof(rage_Harness));
    harness->pb = pb;
    harness->elem = elem;
    uint8_t n_views = view_count_for_type(elem->cet->type);
    // FIXME: Error handling
    harness->interpolators = RAGE_SUCCESS_VALUE(interpolators_for(
        pb->sample_rate, &elem->cet->controls, controls, n_views));
    harness->views = rage_proc_block_initialise_views(elem->cet, harness);
    // FIXME: could be more efficient, and not add this if not required
    harness->truck = rage_support_convoy_mount(
        pb->convoy, elem, harness->views.prep, harness->views.clean);
    harness->ports.controls = harness->views.rt;
    // FIXME: Should there be one of these per harness?
    harness->ports.inputs = calloc(elem->cet->inputs.len, sizeof(void *));
    harness->ports.outputs = calloc(elem->cet->outputs.len, sizeof(void *));
    rage_RtBits const * old = rage_rt_crit_update_start(harness->pb->syncy);
    rage_RtBits * new = malloc(sizeof(rage_RtBits));
    *new = *old;
    RAGE_ARRAY_INIT(&new->steps, old->steps.len + 1, i) {
        if (i != old->steps.len) {
            new->steps.items[i] = old->steps.items[i];
        } else {
            rage_ProcStep * proc_step = &new->steps.items[i];
            // FIXME: Everything reads & writes only to the first buffer
            proc_step->in_buffer_allocs = calloc(elem->cet->inputs.len, sizeof(uint32_t));
            proc_step->out_buffer_allocs = calloc(elem->cet->outputs.len, sizeof(uint32_t));
            proc_step->out_buffer_allocs[0] = 1;
            proc_step->harness = harness;
        }
    }
    // FIXME: Leaks proc_step array
    free(rage_rt_crit_update_finish(harness->pb->syncy, new));
    return RAGE_SUCCESS(rage_MountResult, harness);
}

static void rage_remove_connections_for(
        rage_PBConnection ** initial, rage_Harness * tgt) {
    rage_PBConnection * c, * prev_con = NULL;
    for (c = *initial; c != NULL; c = c->next) {
        if (c->source_harness == tgt || c->sink_harness == tgt) {
            if (prev_con) {
                prev_con->next = c->next;
            } else {
                *initial = c;
            }
            break;
        }
        prev_con = c;
    }
    free(c);
}

void rage_proc_block_unmount(rage_Harness * harness) {
    rage_remove_connections_for(&harness->pb->cons, harness);
    rage_RtBits const * old = rage_rt_crit_update_start(harness->pb->syncy);
    rage_RtBits * new = malloc(sizeof(rage_RtBits));
    *new = *old;
    rage_ProcStep * proc_step = NULL;
    for (uint32_t i = 0; i < old->steps.len; i++) {
        if (old->steps.items[i].harness == harness) {
            proc_step = &old->steps.items[i];
        } else {
            uint32_t const j = (proc_step == NULL) ? i : i - 1;
            new->steps.items[j] = old->steps.items[i];
        }
    }
    new->steps.len = old->steps.len - 1;
    // FIXME: Leaks the proc_step array
    free(rage_rt_crit_update_finish(harness->pb->syncy, new));
    rage_support_convoy_unmount(harness->truck);
    free(proc_step->in_buffer_allocs);
    free(proc_step->out_buffer_allocs);
    free(harness->ports.inputs);
    free(harness->ports.outputs);
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
    rage_RtBits const * old = rage_rt_crit_update_start(pb->syncy);
    rage_RtBits * new = malloc(sizeof(rage_RtBits));
    *new = *old;
    new->transp = state;
    free(rage_rt_crit_update_finish(pb->syncy, new));
    rage_support_convoy_transport_state_changed(pb->convoy, state);
}

rage_Error rage_proc_block_transport_seek(rage_ProcBlock * pb, rage_FrameNo target) {
    rage_Error e = RAGE_OK;
    rage_RtBits const * rtd = rage_rt_crit_freeze(pb->syncy);
    if (rtd->transp == RAGE_TRANSPORT_ROLLING) {
        e = RAGE_ERROR("Seek whilst rolling not implemented");
    } else {
        e = rage_support_convoy_transport_seek(pb->convoy, target);
        if (!RAGE_FAILED(e)) {
            for (uint32_t i = 0; i < rtd->steps.len; i++) {
                uint32_t const n_controls =
                    rtd->steps.items[i].harness->elem->cet->controls.len;
                for (uint32_t j = 0; j < n_controls; j++) {
                    rage_interpolated_view_seek(rtd->steps.items[i].harness->ports.controls[j], target);
                }
            }
        }
    }
    rage_rt_crit_thaw(pb->syncy);
    return e;
}

void rage_proc_block_process(
        rage_BackendInterface const * b, uint32_t n_frames, void * data) {
    rage_ProcBlock * pb = data;
    rage_RtBits * rtd = rage_rt_crit_data_latest(pb->syncy);
    rage_backend_get_buffers(
        b, n_frames, rtd->all_buffers, rtd->all_buffers + pb->n_inputs);
    for (uint32_t i = 0; i < rtd->steps.len; i++) {
        rage_ProcStep * step = &rtd->steps.items[i];
        for (uint32_t j = 0; j < step->harness->elem->cet->inputs.len; j++) {
            step->harness->inputs[j] =
                rtd->all_buffers[step->in_buffer_allocs[j]];
        }
        for (uint32_t j = 0; j < step->harness->elem->cet->outputs.len; j++) {
            step->harness->outputs[j] =
                rtd->all_buffers[step->out_buffer_allocs[j]];
        }
        rage_element_process(
            step->harness->elem, rtd->transp, n_frames, &step->harness->ports);
    }
    if (rtd->transp == RAGE_TRANSPORT_ROLLING) {
        rage_countdown_add(pb->rolling_countdown, -1);
    }
}

rage_Error rage_proc_block_connect(
        rage_ProcBlock * pb,
        rage_Harness * source, uint32_t source_idx,
        rage_Harness * sink, uint32_t sink_idx) {
    rage_PBConnection new = {
        .source_harness = source, .source_idx = source_idx,
        .sink_harness = sink, .sink_idx = sink_idx,
        .next = pb->cons
    };
    return RAGE_ERROR("Actual connection handling not implemented");
    rage_PBConnection * newp = malloc(sizeof(rage_PBConnection));
    *newp = new;
    pb->cons = newp;
    return RAGE_OK;
}

static void rage_remove_connection(
        rage_PBConnection ** initial,
        rage_Harness * source, uint32_t source_idx,
        rage_Harness * sink, uint32_t sink_idx) {
    rage_PBConnection * c, * prev_con = NULL;
    for (c = *initial; c != NULL; c = c->next) {
        if (
                c->source_harness == source && c->source_idx == source_idx &&
                c->sink_harness == sink && c->sink_idx == sink_idx) {
            if (prev_con) {
                prev_con->next = c->next;
            } else {
                *initial = c;
            }
            break;
        }
        prev_con = c;
    }
    free(c);
}

rage_Error rage_proc_block_disconnect(
        rage_ProcBlock * pb,
        rage_Harness * source, uint32_t source_idx,
        rage_Harness * sink, uint32_t sink_idx) {
    rage_remove_connection(&pb->cons, source, source_idx, sink, sink_idx);
    return RAGE_ERROR("Defo not implemented");
}

static char * desc[] = {
    "port"
};

static char * desc2[] = {
    "portly"
};

rage_BackendConfig rage_proc_block_get_backend_config(rage_ProcBlock * pb) {
    return (rage_BackendConfig) {
        .sample_rate = pb->sample_rate,
        .buffer_size = 1024, // FIXME: FIXED BUFFER SIZE
        .ports = {
            .inputs = {.len = 1, .items = desc},
            .outputs = {.len = 1, .items = desc2}
        },
        .process = rage_proc_block_process,
        .data = pb
    };
}
