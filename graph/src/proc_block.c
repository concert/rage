#include "proc_block.h"
#include "srt.h"
#include "element_helpers.h"
#include "rtcrit.h"
#include "binding_interface.h"
#include "set.h"
#include "buffer_pile.h"
#include "depmap.h"
#include <stdlib.h>


typedef struct {
    rage_InterpolatedView ** prep;
    rage_InterpolatedView ** clean;
    rage_InterpolatedView ** rt;
} rage_ProcBlockViews;

struct rage_Harness {
    rage_Element * elem;
    // FIXME: Don't actually need a ports per elem (but 1 of the largest size)?
    RAGE_EMBED_STRUCT(rage_Ports, ports);
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

typedef RAGE_ARRAY(rage_ProcStep) rage_ProcSteps;

typedef struct rage_ExternalOut {
    uint32_t primary;
    RAGE_ARRAY(uint32_t);
    struct rage_ExternalOut * next;
} rage_ExternalOut;

typedef struct {
    rage_TransportState transp;
    void ** all_buffers;
    rage_ProcSteps steps;
    rage_ExternalOut * ext_outs;
} rage_RtBits;

struct rage_ProcBlock {
    uint32_t sample_rate;
    uint32_t period_size;
    rage_BackendPorts be_ports;
    rage_Queue * evt_q;
    rage_SupportConvoy * convoy;
    rage_Countdown * rolling_countdown;
    rage_RtCrit * syncy;
    rage_DepMap * cons;
    rage_BufferAllocs * allocs;
    void * silent_buffer;
    void * unrouted_buffer;
    uint32_t min_dynamic_buffer;
};

rage_ProcBlock * rage_proc_block_new(
        uint32_t sample_rate, uint32_t period_size,
        rage_BackendPorts ports, rage_TransportState transp_state, rage_Queue * evt_q) {
    rage_ProcBlock * pb = malloc(sizeof(rage_ProcBlock));
    pb->cons = rage_depmap_new();
    pb->sample_rate = sample_rate;
    pb->period_size = period_size;
    pb->be_ports = ports;
    pb->min_dynamic_buffer = 2 + ports.inputs.len + ports.outputs.len;
    pb->evt_q = evt_q;
    pb->convoy = rage_support_convoy_new(period_size, transp_state, evt_q);
    pb->rolling_countdown = rage_support_convoy_get_countdown(pb->convoy);
    rage_RtBits * rtb = malloc(sizeof(rage_RtBits));
    rtb->transp = transp_state;
    rtb->all_buffers = calloc(pb->min_dynamic_buffer, sizeof(void *));
    rtb->steps.len = 0;
    rtb->steps.items = NULL;
    pb->syncy = rage_rt_crit_new(rtb);
    pb->allocs = rage_buffer_allocs_new(period_size * sizeof(float));
    pb->silent_buffer = calloc(period_size, sizeof(float));
    rtb->all_buffers[0] = pb->silent_buffer;
    pb->unrouted_buffer = calloc(period_size, sizeof(float));
    rtb->all_buffers[1] = pb->unrouted_buffer;
    rtb->ext_outs = NULL;
    return pb;
}

static void rage_harness_free(rage_Harness * harness) {
    rage_support_convoy_unmount(harness->truck);
    free(harness->ports.inputs);
    free(harness->ports.outputs);
    for (uint32_t i = 0; i < harness->elem->cet->spec.controls.len; i++) {
        rage_interpolator_free(&harness->elem->cet->spec.controls.items[i], harness->interpolators[i]);
    }
    free(harness->interpolators);
    free(harness->views.prep);
    free(harness->views.clean);
    free(harness->views.rt);
    free(harness);
}

static void rage_procsteps_free(rage_ProcSteps * steps) {
    for (uint32_t i = 0; i < steps->len; i++) {
        free(steps->items[i].in_buffer_allocs);
        free(steps->items[i].out_buffer_allocs);
        rage_harness_free(steps->items[i].harness);
    }
    free(steps->items);
}

static void rage_ext_outs_free(rage_ExternalOut * ext) {
    while (ext != NULL) {
        rage_ExternalOut * next = ext->next;
        free(ext->items);
        free(ext);
        ext = next;
    }
}

void rage_proc_block_free(rage_ProcBlock * pb) {
    rage_support_convoy_free(pb->convoy);
    rage_RtBits * rtb = rage_rt_crit_free(pb->syncy);
    free(rtb->all_buffers);
    rage_procsteps_free(&rtb->steps);
    rage_ext_outs_free(rtb->ext_outs);
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

static uint32_t * rage_alloc_int_array(uint32_t n_ints, uint32_t value) {
    uint32_t * a = malloc(n_ints * sizeof(uint32_t));
    while (n_ints--) {
        a[n_ints] = value;
    }
    return a;
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
        pb->sample_rate, pb->evt_q, &elem->cet->controls, controls, n_views));
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
            proc_step->in_buffer_allocs = calloc(elem->cet->inputs.len, sizeof(uint32_t));
            proc_step->out_buffer_allocs = rage_alloc_int_array(elem->cet->outputs.len, 1);
            proc_step->harness = harness;
        }
    }
    rage_RtBits * rt_to_free = rage_rt_crit_update_finish(harness->pb->syncy, new);
    free(rt_to_free->steps.items);
    free(rt_to_free);
    return RAGE_SUCCESS(rage_MountResult, harness);
}

void rage_proc_block_unmount(rage_Harness * harness) {
    rage_remove_connections_for(harness->pb->cons, harness);
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
        rage_TimeSeries const * const new_controls) {
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

// This is a bit of a nut/sledgehammer, it runs a fairly cheap operation on all
// interpolators to force any time series changes to be picked up:
static void pickup_new_timeseries(rage_ProcSteps * steps) {
    for (uint32_t i = 0; i < steps->len; i++) {
        rage_Harness * h = steps->items[i].harness;
        for (uint32_t j = 0; j < h->elem->cet->spec.controls.len; j++) {
            rage_interpolated_view_advance(h->ports.controls[j], 0);
        }
    }
}

void rage_proc_block_process(
        rage_BackendInterface const * b, uint32_t const n_frames, void * data) {
    rage_ProcBlock * pb = data;
    rage_RtBits * rtd = rage_rt_crit_data_latest(pb->syncy);
    rage_backend_get_buffers(
        b, n_frames, rtd->all_buffers + 2, rtd->all_buffers + pb->be_ports.inputs.len + 2);
    if (rtd->transp == RAGE_TRANSPORT_STOPPED) {
        pickup_new_timeseries(&rtd->steps);
    }
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
    for (rage_ExternalOut * d = rtd->ext_outs; d != NULL; d = d->next) {
        for (uint32_t i = 0; i < d->len; i++) {
            memcpy(
                rtd->all_buffers[d->primary], rtd->all_buffers[d->items[i]],
                n_frames * sizeof(float));
        }
    }
    if (rtd->transp == RAGE_TRANSPORT_ROLLING) {
        rage_countdown_add(pb->rolling_countdown, -1);
    }
}

typedef RAGE_OR_ERROR(rage_ProcSteps) rage_OrderedProcSteps;
RAGE_SET_OPS(Step, step, rage_ProcStep *)

typedef struct {
    uint32_t idx;
    rage_ProcStep * step;
} rage_StepIdx;

static rage_StepIdx rage_step_for(
        rage_ProcSteps const * steps, rage_Harness const * const harness) {
    rage_StepIdx rv = {.idx = UINT32_MAX, .step = NULL};
    for (uint32_t i = 0; i < steps->len; i++) {
        if (steps->items[i].harness == harness) {
            rv.idx = i;
            rv.step = &steps->items[i];
            break;
        }
    }
    return rv;
}

static rage_StepSet ** rage_step_deps(
        rage_ProcSteps const * steps, rage_DepMap const * cons) {
    rage_StepSet ** deps = calloc(steps->len, sizeof(rage_StepSet *));
    for (uint32_t i = 0; i < steps->len; i++) {
        deps[i] = rage_step_set_new();
    }
    for (uint32_t i = 0; i < cons->len; i++) {
        if (cons->items[i].src.harness != NULL && cons->items[i].sink.harness != NULL) {
            rage_StepIdx const si = rage_step_for(steps, cons->items[i].sink.harness);
            rage_StepIdx const sj = rage_step_for(steps, cons->items[i].src.harness);
            rage_StepSet * const extended_deps = rage_step_set_add(deps[si.idx], sj.step);
            rage_step_set_free(deps[si.idx]);
            deps[si.idx] = extended_deps;
        }
    }
    return deps;
}

static void rage_free_steps_deps(rage_ProcSteps const * steps, rage_StepSet ** deps) {
    for (uint32_t i = 0; i < steps->len; i++) {
        rage_step_set_free(deps[i]);
    }
    free(deps);
}

static rage_OrderedProcSteps rage_order_proc_steps(
        rage_ProcSteps const * steps, rage_DepMap const * cons) {
    rage_StepSet ** deps = rage_step_deps(steps, cons);
    rage_StepSet * resolved = rage_step_set_new();
    bool failed = false;
    rage_ProcSteps ordered_steps;
    RAGE_ARRAY_INIT(&ordered_steps, steps->len, resolved_idx) {
        failed = true;
        for (uint32_t i = 0; i < steps->len; i++) {
            if (rage_step_set_contains(resolved, &steps->items[i])) {
                continue;
            }
            if (rage_step_set_is_weak_subset(deps[i], resolved)) {
                ordered_steps.items[resolved_idx] = steps->items[i];
                ordered_steps.items[resolved_idx].in_buffer_allocs = calloc(
                    steps->items[i].harness->elem->cet->inputs.len,
                    sizeof(uint32_t));
                ordered_steps.items[resolved_idx].out_buffer_allocs =
                    rage_alloc_int_array(
                        steps->items[i].harness->elem->cet->outputs.len, 1);
                rage_StepSet * const new_resolved = rage_step_set_add(
                    resolved, &steps->items[i]);
                rage_step_set_free(resolved);
                resolved = new_resolved;
                failed = false;
                break;
            }
        }
        if (failed) {
            break;
        }
    }
    rage_free_steps_deps(steps, deps);
    rage_step_set_free(resolved);
    if (failed) {
        free(ordered_steps.items);
        return RAGE_FAILURE(rage_OrderedProcSteps, "Circular connections");
    }
    return RAGE_SUCCESS(rage_OrderedProcSteps, ordered_steps);
}

typedef struct rage_ConnTarget {
    rage_Harness * tgt_harness;
    uint32_t tgt_idx;
    struct rage_ConnTarget * next;
} rage_ConnTarget;

typedef struct rage_AssignedConnection {
    uint32_t assignment;
    struct rage_AssignedConnection * next;
    rage_Harness * source_harness;
    uint32_t source_idx;
    rage_ConnTarget * con_tgt;
} rage_AssignedConnection;

static rage_AssignedConnection * rage_cons_from(
        rage_DepMap const * cons, rage_Harness const * const tgt) {
    rage_AssignedConnection * cons_from = NULL;
    for (uint32_t i = 0; i < cons->len; i++) {
        if (cons->items[i].src.harness == tgt) {
            bool create = true;
            rage_ConnTarget * new_tgt = malloc(sizeof(rage_ConnTarget));
            new_tgt->tgt_harness = cons->items[i].sink.harness;
            new_tgt->tgt_idx = cons->items[i].sink.idx;
            for (rage_AssignedConnection * c = cons_from; c != NULL; c = c->next) {
                if (c->source_harness == tgt && c->source_idx == cons->items[i].src.idx) {
                    new_tgt->next = c->con_tgt;
                    c->con_tgt = new_tgt;
                    create = false;
                    break;
                }
            }
            if (create) {
                rage_AssignedConnection * new_con = malloc(sizeof(rage_AssignedConnection));
                new_con->source_harness = cons->items[i].src.harness;
                new_con->source_idx = cons->items[i].src.idx;
                new_tgt->next = NULL;
                new_con->con_tgt = new_tgt;
                new_con->next = cons_from;
                cons_from = new_con;
            }
        }
    }
    return cons_from;
}

static uint32_t rage_lowest_avail_assign(
        uint32_t i, rage_AssignedConnection const * const assigned_cons,
        uint32_t * const highest_assignment) {
    bool found;
    do {
        found = false;
        for (rage_AssignedConnection const * ac = assigned_cons; ac != NULL; ac = ac->next) {
            if (i == ac->assignment) {
                i++;
                found = true;
            }
        }
    } while (found);
    if (i  > *highest_assignment) {
        *highest_assignment = i;
    }
    return i;
}

static uint32_t rage_step_idx(rage_ProcSteps const * steps, rage_Harness const * const tgt) {
    for (uint32_t i = 0; i < steps->len; i++) {
        if (steps->items[i].harness == tgt) {
            return i;
        }
    }
    return UINT32_MAX;
}

static rage_AssignedConnection * rage_remove_cons_targetted(
        rage_AssignedConnection * assigned_cons, rage_Harness const * const tgt) {
    rage_AssignedConnection * remaining_cons = NULL;
    while (assigned_cons != NULL) {
        rage_ConnTarget * new_targets = NULL;
        while (assigned_cons->con_tgt != NULL) {
            rage_ConnTarget * next_con_tgt = assigned_cons->con_tgt->next;
            if (assigned_cons->con_tgt->tgt_harness == tgt) {
                free(assigned_cons->con_tgt);
            } else {
                assigned_cons->con_tgt->next = new_targets;
                new_targets = assigned_cons->con_tgt;
            }
            assigned_cons->con_tgt = next_con_tgt;
        }
        rage_AssignedConnection * next_con = assigned_cons->next;
        if (new_targets == NULL) {
            free(assigned_cons);
        } else {
            assigned_cons->con_tgt = new_targets;
            assigned_cons->next = remaining_cons;
            remaining_cons = assigned_cons;
        }
        assigned_cons = next_con;
    }
    return remaining_cons;
}

static rage_ExternalOut * rage_get_ext_outs(uint32_t const output_offset, rage_AssignedConnection * c) {
    unsigned n_ext = 0;
    for (rage_ConnTarget const * t = c->con_tgt; t != NULL; t = t->next) {
        if (t->tgt_harness == NULL) {
            n_ext++;
        }
    }
    if (n_ext) {
        rage_ExternalOut * ext_out = malloc(sizeof(rage_ExternalOut));
        ext_out->len = n_ext - 1;
        if (ext_out->len) {
            ext_out->items = calloc(ext_out->len, sizeof(uint32_t));
        }
        bool primary_set = false;
        uint32_t i = 0;
        for (rage_ConnTarget const * t = c->con_tgt; t != NULL; t = t->next) {
            if (t->tgt_harness == NULL) {
                uint32_t const all_idx = output_offset + t->tgt_idx;
                if (primary_set) {
                    ext_out->items[i++] = all_idx;
                } else {
                    ext_out->primary = all_idx;
                    primary_set = true;
                }
            }
        }
        return ext_out;
    } else {
        return NULL;
    }
}

static void rage_pb_init_all_buffers(rage_ProcBlock const * pb, rage_RtBits * rt, uint32_t highest_assignment) {
    rt->all_buffers = calloc(highest_assignment + 1, sizeof(void *));
    rt->all_buffers[0] = pb->silent_buffer;
    rt->all_buffers[1] = pb->unrouted_buffer;
    rage_BuffersInfo const * buffer_info = rage_buffer_allocs_get_buffers_info(pb->allocs);
    memcpy(&rt->all_buffers[pb->min_dynamic_buffer], buffer_info->buffers, buffer_info->n_buffers * sizeof(void *));
}

struct rage_ConTrans {
    rage_ProcBlock * pb;
    rage_DepMap * cons;
    rage_RtBits const * old_rt;
    rage_ProcSteps steps;
    uint32_t highest_assignment;
    bool empty_trans;
};

rage_ConTrans * rage_proc_block_con_trans_start(rage_ProcBlock * pb) {
    rage_ConTrans * ct = malloc(sizeof(rage_ConTrans));
    ct->pb = pb;
    ct->cons = pb->cons;
    ct->old_rt = rage_rt_crit_update_start(pb->syncy);
    ct->steps = ct->old_rt->steps;
    ct->empty_trans = true;
    return ct;
}

void rage_proc_block_con_trans_commit(rage_ConTrans * trans) {
    if (trans->empty_trans) {
        rage_proc_block_con_trans_abort(trans);
        return;
    }
    rage_AssignedConnection * assigned_cons = NULL;
    uint32_t highest_assignment = trans->pb->min_dynamic_buffer - 1;
    rage_ExternalOut * ext_outs = NULL;
    for (uint32_t i = 0; i < trans->steps.len; i++) {
        rage_AssignedConnection * step_cons = rage_cons_from(trans->cons, trans->steps.items[i].harness);
        while (step_cons != NULL) {
            rage_AssignedConnection * const c = step_cons;
            step_cons = step_cons->next;
            rage_ExternalOut * ext = rage_get_ext_outs(trans->pb->be_ports.inputs.len + 2, c);
            if (ext != NULL) {
                c->assignment = ext->primary;
                if (ext->len) {
                    ext->next = ext_outs;
                    ext_outs = ext;
                } else {
                    free(ext);
                }
            } else {
                c->assignment = rage_lowest_avail_assign(
                    trans->pb->min_dynamic_buffer, assigned_cons, &highest_assignment);
            }
            c->next = assigned_cons;
            assigned_cons = c;
            trans->steps.items[i].out_buffer_allocs[c->source_idx] = c->assignment;
            for (rage_ConnTarget * ct = c->con_tgt; ct != NULL; ct = ct->next) {
                if (ct->tgt_harness == NULL) {  // External connection
                    continue;  // FIXME: should the external and internal connections be mixed up?
                }
                uint32_t const step_idx = rage_step_idx(&trans->steps, ct->tgt_harness);
                trans->steps.items[step_idx].in_buffer_allocs[ct->tgt_idx] = c->assignment;
            }
        }
        assigned_cons = rage_remove_cons_targetted(assigned_cons, trans->steps.items[i].harness);
    }
    rage_BufferAllocs * const old_allocs = trans->pb->allocs;
    trans->pb->allocs = rage_buffer_allocs_alloc(
        old_allocs, highest_assignment - (trans->pb->min_dynamic_buffer - 1));
    rage_RtBits * new_rt = malloc(sizeof(rage_RtBits));
    new_rt->transp = trans->old_rt->transp;
    new_rt->steps = trans->steps;
    new_rt->ext_outs = ext_outs;
    rage_pb_init_all_buffers(trans->pb, new_rt, highest_assignment);
    rage_RtBits * replaced = rage_rt_crit_update_finish(trans->pb->syncy, new_rt);
    rage_buffer_allocs_free(old_allocs);
    free(replaced->steps.items);
    free(replaced->all_buffers);
    rage_ext_outs_free(replaced->ext_outs);
    free(replaced);
    free(trans);
}

void rage_proc_block_con_trans_abort(rage_ConTrans * ct) {
    if (!ct->empty_trans) {
        free(ct->steps.items);
        rage_depmap_free(ct->cons);
    }
    rage_rt_crit_update_abort(ct->pb->syncy);
    free(ct);
}

static rage_Error rage_proc_block_recalculate_routing(rage_ConTrans * trans, rage_DepMap * dm) {
    rage_OrderedProcSteps ops = rage_order_proc_steps(&trans->steps, dm);
    if (RAGE_FAILED(ops)) {
        rage_depmap_free(dm);
        return RAGE_FAILURE_CAST(rage_Error, ops);
    }
    if (!trans->empty_trans) {
        free(trans->steps.items);
        rage_depmap_free(trans->cons);
    }
    trans->empty_trans = false;
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

rage_BackendConfig rage_proc_block_get_backend_config(rage_ProcBlock * pb) {
    return (rage_BackendConfig) {
        .sample_rate = pb->sample_rate,
        .buffer_size = pb->period_size,
        .ports = pb->be_ports,
        .process = rage_proc_block_process,
        .data = pb
    };
}
