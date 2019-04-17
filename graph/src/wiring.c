#include "wiring.h"
#include "set.h"

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

uint32_t * rage_alloc_int_array(uint32_t n_ints, uint32_t value) {
    uint32_t * a = malloc(n_ints * sizeof(uint32_t));
    while (n_ints--) {
        a[n_ints] = value;
    }
    return a;
}

void rage_proc_step_init(rage_ProcStep * step, rage_Harness * harness) {
    step->harness = harness;
    step->in_buffer_allocs = calloc(
        rage_harness_n_ins(harness), sizeof(uint32_t));
    step->out_buffer_allocs = rage_alloc_int_array(
        rage_harness_n_outs(harness), 1);
}

void rage_proc_steps_destroy(rage_ProcSteps * steps) {
    for (uint32_t i = 0; i < steps->len; i++) {
        free(steps->items[i].in_buffer_allocs);
        free(steps->items[i].out_buffer_allocs);
    }
    free(steps->items);
    steps->items = NULL;
    steps->len = 0;
}

void rage_ext_outs_free(rage_ExternalOut * ext) {
    while (ext != NULL) {
        rage_ExternalOut * next = ext->next;
        free(ext->items);
        free(ext);
        ext = next;
    }
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

rage_OrderedProcSteps rage_order_proc_steps(
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
                rage_proc_step_init(
                    &ordered_steps.items[resolved_idx],
                    steps->items[i].harness);
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

uint32_t assign_buffers(
        rage_DepMap const * cons, rage_ProcSteps const * steps,
        uint32_t const n_ext_ins, uint32_t const min_dynamic_buffer,
        rage_ExternalOut ** ext_outs) {
    rage_AssignedConnection * assigned_cons = NULL;
    uint32_t highest_assignment = min_dynamic_buffer - 1;
    *ext_outs = NULL;
    for (uint32_t i = 0; i < steps->len; i++) {
        rage_AssignedConnection * step_cons = rage_cons_from(cons, steps->items[i].harness);
        while (step_cons != NULL) {
            rage_AssignedConnection * const c = step_cons;
            step_cons = step_cons->next;
            rage_ExternalOut * ext = rage_get_ext_outs(n_ext_ins + 2, c);
            if (ext != NULL) {
                c->assignment = ext->primary;
                if (ext->len) {
                    ext->next = *ext_outs;
                    *ext_outs = ext;
                } else {
                    free(ext);
                }
            } else {
                c->assignment = rage_lowest_avail_assign(
                    min_dynamic_buffer, assigned_cons, &highest_assignment);
            }
            c->next = assigned_cons;
            assigned_cons = c;
            steps->items[i].out_buffer_allocs[c->source_idx] = c->assignment;
            for (rage_ConnTarget * ct = c->con_tgt; ct != NULL; ct = ct->next) {
                if (ct->tgt_harness == NULL) {  // External connection
                    continue;  // FIXME: should the external and internal connections be mixed up?
                }
                uint32_t const step_idx = rage_step_idx(steps, ct->tgt_harness);
                steps->items[step_idx].in_buffer_allocs[ct->tgt_idx] = c->assignment;
            }
        }
        assigned_cons = rage_remove_cons_targetted(assigned_cons, steps->items[i].harness);
    }
    return highest_assignment;
}
