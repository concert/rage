#include "proc_block.h"
#include "srt.h"
#include "element_helpers.h"
#include "rtcrit.h"
#include "binding_interface.h"
#include "buffer_pile.h"
#include "depmap.h"
#include <stdlib.h>
#include "wiring.h"

typedef struct {
    rage_InterpolatedView ** prep;
    rage_InterpolatedView ** clean;
    rage_InterpolatedView ** rt;
} rage_ProcBlockViews;

struct rage_Harness {
    rage_Element * elem;
    rage_SupportTruck * truck;
    rage_Interpolator ** interpolators;
    rage_ProcBlockViews views;
    rage_ProcBlock * pb;
};

typedef struct {
    rage_TransportState transp;
    uint32_t max_inputs;
    uint32_t max_outputs;
    RAGE_EMBED_STRUCT(rage_Ports, ports);
    void ** all_buffers;
    rage_ProcSteps steps;
    rage_ExternalOut * ext_outs;
    uint32_t ext_revision;
    uint32_t n_ext_ins;
    uint32_t n_ext_outs;
    uint32_t min_dynamic_buffer;
} rage_RtBits;

struct rage_ProcBlock {
    uint32_t sample_rate;
    uint32_t period_size;
    rage_Queue * evt_q;
    rage_SupportConvoy * convoy;
    rage_Countdown * rolling_countdown;
    rage_RtCrit * syncy;
    rage_DepMap * cons;
    rage_BufferAllocs * allocs;
    void * silent_buffer;
    void * unrouted_buffer;
    rage_BackendHooks hooks;
};

struct rage_ConTrans {
    rage_ProcBlock * pb;
    rage_DepMap * cons;
    rage_RtBits const * old_rt;
    rage_ProcSteps steps;
};

static void rage_pb_init_all_buffers(rage_ProcBlock const * pb, rage_RtBits * rt, uint32_t highest_assignment) {
    rt->all_buffers = calloc(highest_assignment + 1, sizeof(void *));
    rt->all_buffers[0] = pb->silent_buffer;
    rt->all_buffers[1] = pb->unrouted_buffer;
    rage_BuffersInfo const * buffer_info = rage_buffer_allocs_get_buffers_info(pb->allocs);
    memcpy(&rt->all_buffers[rt->min_dynamic_buffer], buffer_info->buffers, buffer_info->n_buffers * sizeof(void *));
}

static void rage_proc_block_set_externals(
            void * data, uint32_t const ext_revision,
            uint32_t const n_ins, uint32_t const n_outs) {
    rage_ProcBlock * pb = data;
    rage_RtBits const * old_rt = rage_rt_crit_update_start(pb->syncy);
    rage_remove_connections_for(pb->cons, NULL, n_ins, n_outs);
    rage_RtBits * new_rt = malloc(sizeof(rage_RtBits));
    *new_rt = *old_rt;
    new_rt->ext_revision = ext_revision;
    new_rt->n_ext_ins = n_ins;
    new_rt->n_ext_outs = n_outs;
    new_rt->min_dynamic_buffer = 2 + n_ins + n_outs;
    RAGE_ARRAY_INIT(&new_rt->steps, old_rt->steps.len, i) {
        rage_proc_step_init(
            &new_rt->steps.items[i], old_rt->steps.items[i].harness);
    }
    uint32_t const highest_assignment = assign_buffers(
        pb->cons, &new_rt->steps, n_ins, new_rt->min_dynamic_buffer,
        &new_rt->ext_outs);
    rage_pb_init_all_buffers(pb, new_rt, highest_assignment);
    rage_Ticking * tf = pb->hooks.tick_ensure_start(pb->hooks.b);
    rage_RtBits * replaced = rage_rt_crit_update_finish(pb->syncy, new_rt);
    pb->hooks.tick_ensure_end(tf);
    free(replaced->all_buffers);
    rage_proc_steps_destroy(&replaced->steps);
    rage_ext_outs_free(replaced->ext_outs);
    free(replaced);
}

static void rage_harness_free(rage_Harness * harness) {
    rage_support_convoy_unmount(harness->truck);
    for (uint32_t i = 0; i < harness->elem->type->spec.controls.len; i++) {
        rage_interpolator_free(&harness->elem->type->spec.controls.items[i], harness->interpolators[i]);
    }
    free(harness->interpolators);
    free(harness->views.prep);
    free(harness->views.clean);
    free(harness->views.rt);
    free(harness);
}

static void rage_procsteps_free(rage_ProcSteps * steps) {
    for (uint32_t i = 0; i < steps->len; i++) {
        rage_harness_free(steps->items[i].harness);
    }
    rage_proc_steps_destroy(steps);
}

void rage_proc_block_free(rage_ProcBlock * pb) {
    rage_support_convoy_free(pb->convoy);
    rage_RtBits * rtb = rage_rt_crit_free(pb->syncy);
    free(rtb->all_buffers);
    rage_procsteps_free(&rtb->steps);
    rage_ext_outs_free(rtb->ext_outs);
    free(rtb->inputs);
    free(rtb->outputs);
    free(rtb);
    rage_buffer_allocs_free(pb->allocs);
    rage_depmap_free(pb->cons);
    free(pb->silent_buffer);
    free(pb->unrouted_buffer);
    free(pb);
}

rage_Error rage_proc_block_start(rage_ProcBlock * pb) {
    return rage_support_convoy_start(pb->convoy);
}

rage_Error rage_proc_block_stop(rage_ProcBlock * pb) {
    return rage_support_convoy_stop(pb->convoy);
}

static rage_ProcBlockViews rage_proc_block_initialise_views(
        rage_ElementType * type, rage_Harness * harness) {
    rage_ProcBlockViews views;
    views.rt = calloc(
        type->controls.len, sizeof(rage_InterpolatedView *));
    if (type->prep == NULL) {
        views.prep = NULL;
    } else {
        views.prep = calloc(
            type->controls.len, sizeof(rage_InterpolatedView *));
    }
    if (type->clean == NULL) {
        views.clean = NULL;
    } else {
        views.clean = calloc(
            type->controls.len, sizeof(rage_InterpolatedView *));
    }
    for (uint32_t i = 0; i < type->controls.len; i++) {
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
    uint8_t n_views = view_count_for_type(elem->type);
    // FIXME: Error handling
    harness->interpolators = RAGE_SUCCESS_VALUE(interpolators_for(
        pb->sample_rate, pb->evt_q, &elem->type->controls, controls, n_views));
    harness->views = rage_proc_block_initialise_views(elem->type, harness);
    // FIXME: could be more efficient, and not add this if not required
    harness->truck = rage_support_convoy_mount(
        pb->convoy, elem, harness->views.prep, harness->views.clean);
    rage_RtBits const * old = rage_rt_crit_update_start(harness->pb->syncy);
    rage_RtBits * new = malloc(sizeof(rage_RtBits));
    *new = *old;
    RAGE_ARRAY_INIT(&new->steps, old->steps.len + 1, i) {
        if (i != old->steps.len) {
            new->steps.items[i] = old->steps.items[i];
        } else {
            rage_ProcStep * proc_step = &new->steps.items[i];
            proc_step->in_buffer_allocs = calloc(elem->type->inputs.len, sizeof(uint32_t));
            proc_step->out_buffer_allocs = rage_alloc_int_array(elem->type->outputs.len, 1);
            proc_step->harness = harness;
        }
    }
    if (elem->type->inputs.len > old->max_inputs) {
        new->ports.inputs = calloc(elem->type->inputs.len, sizeof(void *));
        new->max_inputs = elem->type->inputs.len;
    } else {
        new->max_inputs = old->max_inputs;
    }
    if (elem->type->outputs.len > old->max_outputs) {
        new->ports.outputs = calloc(elem->type->outputs.len, sizeof(void *));
        new->max_outputs = elem->type->outputs.len;
    } else {
        new->max_outputs = old->max_outputs;
    }
    rage_RtBits * rt_to_free = rage_rt_crit_update_finish(harness->pb->syncy, new);
    free(rt_to_free->steps.items);
    if (rt_to_free->max_inputs < elem->type->inputs.len) {
        free(rt_to_free->ports.inputs);
    }
    if (rt_to_free->max_outputs < elem->type->outputs.len) {
        free(rt_to_free->ports.outputs);
    }
    free(rt_to_free);
    return RAGE_SUCCESS(rage_MountResult, harness);
}

void rage_proc_block_unmount(rage_Harness * harness) {
    rage_remove_connections_for(harness->pb->cons, harness, 0, 0);
    rage_RtBits const * old = rage_rt_crit_update_start(harness->pb->syncy);
    rage_RtBits * new = malloc(sizeof(rage_RtBits));
    *new = *old;
    rage_ProcStep * proc_step = NULL;
    new->steps.items = calloc(old->steps.len - 1, sizeof(rage_ProcStep));
    for (uint32_t i = 0; i < old->steps.len; i++) {
        if (old->steps.items[i].harness == harness) {
            proc_step = &old->steps.items[i];
        } else {
            uint32_t const j = (proc_step == NULL) ? i : i - 1;
            new->steps.items[j] = old->steps.items[i];
        }
    }
    // FIXME: Does not reduce ports allocation if oversize (although since it's
    // only a few pointers maybe not worth reducing it)
    new->steps.len = old->steps.len - 1;
    rage_RtBits * old_rt = rage_rt_crit_update_finish(harness->pb->syncy, new);
    rage_harness_free(proc_step->harness);
    free(proc_step->in_buffer_allocs);
    free(proc_step->out_buffer_allocs);
    free(old_rt->steps.items);
    free(old_rt);
}

rage_NewEventId rage_harness_set_time_series(
        rage_Harness * const harness,
        uint32_t const series_idx,
        rage_TimeSeries const new_controls) {
    rage_RtBits const * rtd = rage_rt_crit_freeze(harness->pb->syncy);
    rage_FrameNo offset;
    if (rtd->transp == RAGE_TRANSPORT_ROLLING) {
        // FIXME: fixed offset (should be derived from sample rate and period size at least)
        // Also if the transport is stopped before the offset is reached then will wedge unswapped
        offset = 4096;
    } else {
        offset = 0;
    }
    rage_rt_crit_thaw(harness->pb->syncy);
    rage_InterpolatedView * first_rt_view = rage_interpolator_get_view(
        harness->interpolators[series_idx], 0);
    rage_FrameNo const change_at = rage_interpolated_view_get_pos(first_rt_view) + offset;
    rage_NewEventId e = rage_interpolator_change_timeseries(
        harness->interpolators[series_idx], new_controls, change_at);
    if (!RAGE_FAILED(e)) {
        // Force an SRT tick when the transport is stopped:
        rage_support_convoy_transport_state_changed(harness->pb->convoy, rtd->transp);
    }
    return e;
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
                    rtd->steps.items[i].harness->elem->type->controls.len;
                for (uint32_t j = 0; j < n_controls; j++) {
                    rage_interpolated_view_seek(rtd->steps.items[i].harness->views.rt[j], target);
                }
            }
        }
    }
    rage_rt_crit_thaw(pb->syncy);
    return e;
}

// This is a bit of a nut/sledgehammer, it runs a fairly cheap operation on all
// interpolators to force any time series changes to be picked up:
static void pickup_new_timeseries(rage_ProcSteps * steps) {
    for (uint32_t i = 0; i < steps->len; i++) {
        rage_Harness * h = steps->items[i].harness;
        for (uint32_t j = 0; j < h->elem->type->spec.controls.len; j++) {
            rage_interpolated_view_advance(h->views.rt[j], 0);
        }
    }
}

static void rage_proc_block_process(uint32_t const n_frames, void * data) {
    rage_ProcBlock * pb = data;
    rage_RtBits * rtd = rage_rt_crit_data_latest(pb->syncy);
    pb->hooks.get_buffers(
        pb->hooks.b, rtd->ext_revision, n_frames,
        rtd->all_buffers + 2, rtd->all_buffers + rtd->n_ext_ins + 2);
    if (rtd->transp == RAGE_TRANSPORT_STOPPED) {
        pickup_new_timeseries(&rtd->steps);
    }
    for (uint32_t i = 0; i < rtd->steps.len; i++) {
        rage_ProcStep * step = &rtd->steps.items[i];
        for (uint32_t j = 0; j < step->harness->elem->type->inputs.len; j++) {
            rtd->inputs[j] = rtd->all_buffers[step->in_buffer_allocs[j]];
        }
        for (uint32_t j = 0; j < step->harness->elem->type->outputs.len; j++) {
            rtd->outputs[j] = rtd->all_buffers[step->out_buffer_allocs[j]];
        }
        rtd->ports.controls = step->harness->views.rt;
        rage_element_process(
            step->harness->elem, rtd->transp, n_frames, &rtd->ports);
    }
    for (rage_ExternalOut * d = rtd->ext_outs; d != NULL; d = d->next) {
        for (uint32_t i = 0; i < d->len; i++) {
            memcpy(
                rtd->all_buffers[d->primary], rtd->all_buffers[d->items[i]],
                n_frames * sizeof(float));
        }
    }
    if (rtd->transp == RAGE_TRANSPORT_ROLLING) {
        rage_countdown_add(pb->rolling_countdown, -n_frames);
    }
}

rage_ConTrans * rage_proc_block_con_trans_start(rage_ProcBlock * pb) {
    rage_ConTrans * ct = malloc(sizeof(rage_ConTrans));
    ct->pb = pb;
    ct->cons = rage_depmap_copy(pb->cons);
    ct->old_rt = rage_rt_crit_update_start(pb->syncy);
    RAGE_ARRAY_INIT(&ct->steps, ct->old_rt->steps.len, i) {
        rage_proc_step_init(
            &ct->steps.items[i], ct->old_rt->steps.items[i].harness);
    }
    return ct;
}

void rage_proc_block_con_trans_commit(rage_ConTrans * trans) {
    rage_ExternalOut * ext_outs;
    uint32_t const highest_assignment = assign_buffers(
        trans->cons, &trans->steps, trans->old_rt->n_ext_ins,
        trans->old_rt->min_dynamic_buffer, &ext_outs);
    rage_BufferAllocs * const old_allocs = trans->pb->allocs;
    rage_RtBits * new_rt = malloc(sizeof(rage_RtBits));
    *new_rt = *trans->old_rt;
    new_rt->steps = trans->steps;
    new_rt->ext_outs = ext_outs;
    rage_depmap_free(trans->pb->cons);
    trans->pb->cons = trans->cons;
    trans->pb->allocs = rage_buffer_allocs_alloc(
        old_allocs, highest_assignment - (trans->old_rt->min_dynamic_buffer - 1));
    rage_pb_init_all_buffers(trans->pb, new_rt, highest_assignment);
    rage_RtBits * replaced = rage_rt_crit_update_finish(trans->pb->syncy, new_rt);
    rage_buffer_allocs_free(old_allocs);
    rage_proc_steps_destroy(&replaced->steps);
    free(replaced->all_buffers);
    rage_ext_outs_free(replaced->ext_outs);
    free(replaced);
    free(trans);
}

void rage_proc_block_con_trans_abort(rage_ConTrans * ct) {
    rage_proc_steps_destroy(&ct->steps);
    rage_depmap_free(ct->cons);
    rage_rt_crit_update_abort(ct->pb->syncy);
    free(ct);
}

static rage_Error rage_proc_block_recalculate_routing(rage_ConTrans * trans, rage_DepMap * dm) {
    rage_OrderedProcSteps ops = rage_order_proc_steps(&trans->steps, dm);
    if (RAGE_FAILED(ops)) {
        rage_depmap_free(dm);
        return RAGE_FAILURE_CAST(rage_Error, ops);
    }
    rage_proc_steps_destroy(&trans->steps);
    rage_depmap_free(trans->cons);
    trans->steps = RAGE_SUCCESS_VALUE(ops);
    trans->cons = dm;
    return RAGE_OK;
}

rage_Error rage_proc_block_connect(
        rage_ConTrans * trans,
        rage_Harness * source, uint32_t source_idx,
        rage_Harness * sink, uint32_t sink_idx) {
    rage_ConnTerminal in_term = {.harness = source, .idx = source_idx};
    rage_ConnTerminal out_term = {.harness = sink, .idx = sink_idx};
    rage_ExtDepMap edm = rage_depmap_connect(trans->cons, in_term, out_term);
    if (RAGE_FAILED(edm)) {
        return RAGE_FAILURE_CAST(rage_Error, edm);
    }
    return rage_proc_block_recalculate_routing(trans, RAGE_SUCCESS_VALUE(edm));
}

rage_Error rage_proc_block_disconnect(
        rage_ConTrans * trans,
        rage_Harness * source, uint32_t source_idx,
        rage_Harness * sink, uint32_t sink_idx) {
    rage_ConnTerminal in_term = {.harness = source, .idx = source_idx};
    rage_ConnTerminal out_term = {.harness = sink, .idx = sink_idx};
    rage_ExtDepMap edm = rage_depmap_disconnect(trans->cons, in_term, out_term);
    if (RAGE_FAILED(edm)) {
        return RAGE_FAILURE_CAST(rage_Error, edm);
    }
    return rage_proc_block_recalculate_routing(trans, RAGE_SUCCESS_VALUE(edm));
}

rage_ProcBlock * rage_proc_block_new(
        rage_TransportState transp_state, rage_Queue * evt_q,
        rage_BackendInterface * bi) {
    rage_ProcBlock * pb = malloc(sizeof(rage_ProcBlock));
    pb->cons = rage_depmap_new();
    pb->sample_rate = rage_backend_get_sample_rate(bi);
    pb->hooks = rage_backend_setup_process(
        bi, pb, rage_proc_block_process, rage_proc_block_set_externals,
        &pb->period_size);
    pb->evt_q = evt_q;
    pb->convoy = rage_support_convoy_new(transp_state, evt_q);
    pb->rolling_countdown = rage_support_convoy_get_countdown(pb->convoy);
    rage_RtBits * rtb = malloc(sizeof(rage_RtBits));
    rtb->transp = transp_state;
    rtb->max_inputs = 0;
    rtb->max_outputs = 0;
    rtb->n_ext_ins = 0;
    rtb->n_ext_outs = 0;
    rtb->min_dynamic_buffer = 2 + rtb->n_ext_ins + rtb->n_ext_ins;
    rtb->controls = NULL;
    rtb->inputs = NULL;
    rtb->outputs = NULL;
    rtb->all_buffers = calloc(rtb->min_dynamic_buffer, sizeof(void *));
    rtb->steps.len = 0;
    rtb->steps.items = NULL;
    rtb->ext_outs = NULL;
    pb->syncy = rage_rt_crit_new(rtb);
    pb->allocs = rage_buffer_allocs_new(pb->period_size * sizeof(float));
    pb->silent_buffer = calloc(pb->period_size, sizeof(float));
    rtb->all_buffers[0] = pb->silent_buffer;
    pb->unrouted_buffer = calloc(pb->period_size, sizeof(float));
    rtb->all_buffers[1] = pb->unrouted_buffer;
    return pb;
}

uint32_t rage_harness_n_ins(rage_Harness * h) {
    return h->elem->type->inputs.len;
}

uint32_t rage_harness_n_outs(rage_Harness * h) {
    return h->elem->type->inputs.len;
}
