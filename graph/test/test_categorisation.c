#include "depmap.h"
#include "categorisation.h"

static rage_Error test_categorise() {
    rage_Error rv = RAGE_OK;
    rage_DepMap * dm = rage_depmap_new();
    rage_HarnessArray ha = {};
    rage_CategorisedHarnesses ch = rage_categorise(ha, dm);
    if (ch.uncategorised.len || ch.in.len || ch.out.len || ch.rt.len) {
        rv = RAGE_ERROR("Conjured a harness from nothing");
    } else {
        ha.items = malloc(2 * sizeof(rage_Harness *));
        rage_Harness * h0 = (rage_Harness *) 0xA0;
        ha.items[0] = h0;
        ha.len = 1;
        rage_categorised_harnesses_free(ch);
        ch = rage_categorise(ha, dm);
        if (
                ch.uncategorised.len != 1 || ch.in.len || ch.out.len ||
                ch.rt.len || ch.uncategorised.items[0] != h0) {
            rv = RAGE_ERROR("Unconnected not classed as uncategorised");
        } else {
            rage_Harness * h1 = (rage_Harness *) 0xA1;
            ha.items[1] = h1;
            ha.len = 2;
            rage_ConnTerminal term0 = {.harness = NULL};
            rage_ConnTerminal term1 = {.harness = h0};
            dm = RAGE_SUCCESS_VALUE(rage_depmap_connect(dm, term0, term1));
            term1.harness = h1;
            dm = RAGE_SUCCESS_VALUE(rage_depmap_connect(dm, term1, term0));
            rage_categorised_harnesses_free(ch);
            ch = rage_categorise(ha, dm);
            if (
                    ch.uncategorised.len || ch.in.len != 1 || ch.out.len != 1 || ch.rt.len ||
                    ch.in.items[0] != h0 || ch.out.items[0] != h1) {
                rv = RAGE_ERROR("Separate bits connected to outside wrong");
            } else {
                term0.harness = h0;
                rage_ExtDepMap edm = rage_depmap_connect(dm, term0, term1);
                if (RAGE_FAILED(edm)) {
                    rv = RAGE_FAILURE_CAST(rage_Error, edm);
                } else {
                    dm = RAGE_SUCCESS_VALUE(edm);
                    rage_categorised_harnesses_free(ch);
                    ch = rage_categorise(ha, dm);
                    if (
                            ch.uncategorised.len || ch.in.len || ch.out.len || ch.rt.len != 2) {
                        rv = RAGE_ERROR("End to end not counted as RT");
                    }
                }
            }
        }
        free(ha.items);
    }
    rage_categorised_harnesses_free(ch);
    rage_depmap_free(dm);
    return rv;
}
