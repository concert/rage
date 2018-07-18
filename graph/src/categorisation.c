#include "depmap.h"
#include "categorisation.h"
#include <stdlib.h>

typedef struct {
    bool ext_in;
    bool ext_out;
} rage_Externs;

static uint32_t rage_harness_idx(
        rage_HarnessArray const * const all_harnesses,
        rage_Harness const * const tgt) {
    for (uint32_t i = 0; i < all_harnesses->len; i++) {
        if (all_harnesses->items[i] == tgt) {
            return i;
        }
    }
    return UINT32_MAX;
}

static void rage_tag_inputs(
        rage_HarnessArray const * const all_harnesses,
        rage_Externs * const externs, rage_DepMap const * deps,
        rage_Harness const * conn_to) {
    rage_ConnTerminals * conn_outs = rage_depmap_outputs(deps, conn_to);
    for (rage_ConnTerminals * o = conn_outs; o != NULL; o = o->next) {
        if (o->term.harness != NULL) {
            uint32_t const harness_idx = rage_harness_idx(all_harnesses, o->term.harness);
            externs[harness_idx].ext_in = true;
            rage_tag_inputs(all_harnesses, externs, deps, o->term.harness);
        }
    }
    rage_conn_terms_free(conn_outs);
}

static void rage_tag_outputs(
        rage_HarnessArray const * const all_harnesses,
        rage_Externs * const externs, rage_DepMap const * deps,
        rage_Harness const * conn_to) {
    rage_ConnTerminals * conn_ins = rage_depmap_inputs(deps, conn_to);
    for (rage_ConnTerminals * i = conn_ins; i != NULL; i = i->next) {
        if (i->term.harness != NULL) {
            uint32_t const harness_idx = rage_harness_idx(all_harnesses, i->term.harness);
            externs[harness_idx].ext_out = true;
            rage_tag_outputs(all_harnesses, externs, deps, i->term.harness);
        }
    }
    rage_conn_terms_free(conn_ins);
}

rage_CategorisedHarnesses rage_categorise(
        rage_HarnessArray all_harnesses, rage_DepMap const * deps) {
    rage_Externs * externs = calloc(all_harnesses.len, sizeof(rage_Externs));
    rage_tag_inputs(&all_harnesses, externs, deps, NULL);
    rage_tag_outputs(&all_harnesses, externs, deps, NULL);
    rage_CategorisedHarnesses rv = {};
    for (uint32_t i = 0; i < all_harnesses.len; i++) {
        rage_HarnessArray * ha;
        if (externs[i].ext_in && externs[i].ext_out) {
            ha = &rv.rt;
        } else if (externs[i].ext_in) {
            ha = &rv.in;
        } else if (externs[i].ext_out) {
            ha = &rv.out;
        } else {
            ha = &rv.uncategorised;
        }
        ha->items = realloc(ha->items, (ha->len + 1) * sizeof(rage_Harness *));
        ha->items[ha->len++] = all_harnesses.items[i];
    }
    free(externs);
    return rv;
}

void rage_categorised_harnesses_free(rage_CategorisedHarnesses ch) {
    free(ch.uncategorised.items);
    free(ch.in.items);
    free(ch.out.items);
    free(ch.rt.items);
}
