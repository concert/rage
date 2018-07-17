#pragma once
#include "macros.h"
#include "proc_block.h"

typedef struct {
    rage_Harness * harness;
    uint32_t idx;
} rage_ConnTerminal;

typedef RAGE_MAYBE(rage_ConnTerminal) rage_MaybeConnTerminal;

typedef struct rage_ConnTerminals {
    rage_ConnTerminal term;
    struct rage_ConnTerminals * next;
} rage_ConnTerminals;

typedef struct rage_DepMap rage_DepMap;

typedef RAGE_OR_ERROR(rage_DepMap *) rage_ExtDepMap;

rage_DepMap * rage_depmap_new();
rage_ExtDepMap rage_depmap_connect(
        rage_DepMap * dm, rage_ConnTerminal input, rage_ConnTerminal output);

rage_MaybeConnTerminal rage_depmap_input_for(
    rage_DepMap const * dm, rage_ConnTerminal const output);
rage_ConnTerminals * rage_depmap_outputs_for(
    rage_DepMap const * dm, rage_ConnTerminal const input);

rage_ConnTerminals * rage_depmap_inputs(
    rage_DepMap const * dm, rage_Harness const * h);
rage_ConnTerminals * rage_depmap_outputs(
    rage_DepMap const * dm, rage_Harness const * h);

void rage_conn_terms_free(rage_ConnTerminals * ct);
