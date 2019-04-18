#pragma once
#include <stdint.h>
#include "macros.h"
#include "depmap.h"
// For Harness definition
#include "proc_block.h"

typedef struct {
    uint32_t * in_buffer_allocs;
    uint32_t * out_buffer_allocs;
    rage_Harness * harness;
} rage_ProcStep;

typedef RAGE_ARRAY(rage_ProcStep) rage_ProcSteps;
typedef RAGE_OR_ERROR(rage_ProcSteps) rage_OrderedProcSteps;

typedef struct rage_ExternalOut {
    uint32_t primary;
    RAGE_ARRAY(uint32_t);
    struct rage_ExternalOut * next;
} rage_ExternalOut;

uint32_t assign_buffers(
    rage_DepMap const * cons, rage_ProcSteps const * steps,
    uint32_t const n_ext_ins, uint32_t const min_dynamic_buffer,
    rage_ExternalOut ** ext_outs);

rage_OrderedProcSteps rage_order_proc_steps(
    rage_ProcSteps const * steps, rage_DepMap const * cons);

void rage_proc_step_init(rage_ProcStep * step, rage_Harness * harness);
void rage_proc_steps_destroy(rage_ProcSteps * steps);
void rage_ext_outs_free(rage_ExternalOut * ext);
uint32_t * rage_alloc_int_array(uint32_t n_ints, uint32_t value);
