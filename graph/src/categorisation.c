#include "depmap.h"
#include <stdlib.h>

typedef RAGE_ARRAY(rage_Harness *) rage_HarnessArray;

typedef struct {
    bool ext_in;
    bool ext_out;
} rage_Flippy;

typedef struct {
    rage_HarnessArray uncategorised;
    rage_HarnessArray in;
    rage_HarnessArray out;
    rage_HarnessArray rt;
} rage_CategorisedHarnesses;

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
        rage_Flippy * const flippy, rage_DepMap const * deps,
        rage_Harness const * conn_to) {
    rage_ConnTerminals * ins = rage_depmap_inputs(deps, conn_to);
    for (rage_ConnTerminals * in = ins; in != NULL; in = in->next) {
        uint32_t harness_idx = rage_harness_idx(all_harnesses, in->term.harness);
        flippy[harness_idx].ext_in = true;
        rage_tag_inputs(all_harnesses, flippy, deps, in->term.harness);
    }
    rage_conn_terms_free(ins);
}

static void rage_tag_outputs(
        rage_HarnessArray const * const all_harnesses,
        rage_Flippy * const flippy, rage_DepMap const * deps,
        rage_Harness const * conn_to) {
    rage_ConnTerminals * outs = rage_depmap_outputs(deps, conn_to);
    for (rage_ConnTerminals * out = outs; out != NULL; out = out->next) {
        uint32_t harness_idx = rage_harness_idx(all_harnesses, out->term.harness);
        flippy[harness_idx].ext_out = true;
        rage_tag_outputs(all_harnesses, flippy, deps, out->term.harness);
    }
    rage_conn_terms_free(outs);
}

rage_CategorisedHarnesses rage_categorise(
        rage_HarnessArray all_harnesses, rage_DepMap const * deps) {
    rage_Flippy * flippy = calloc(all_harnesses.len, sizeof(rage_Flippy));
    rage_tag_inputs(&all_harnesses, flippy, deps, NULL);
    rage_tag_outputs(&all_harnesses, flippy, deps, NULL);
    rage_CategorisedHarnesses rv = {};
    for (uint32_t i = 0; i < all_harnesses.len; i++) {
        rage_HarnessArray * ha;
        if (flippy[i].ext_in && flippy[i].ext_out) {
            ha = &rv.rt;
        } else if (flippy[i].ext_in) {
            ha = &rv.in;
        } else if (flippy[i].ext_out) {
            ha = &rv.out;
        } else {
            ha = &rv.uncategorised;
        }
        ha->items = realloc(ha->items, (ha->len + 1) * sizeof(rage_Harness *));
        ha->items[ha->len++] = all_harnesses.items[i];
    }
    free(flippy);
    return rv;
}
