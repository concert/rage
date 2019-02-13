#include "depmap.h"

rage_Error test_depmap() {
    rage_Error rv = RAGE_OK;
    rage_Harness *a=(void*)0xA, *b=(void*)0xB;
    rage_ConnTerminal
        ta={.harness=a, .idx=0},
        tb={.harness=b, .idx=0};
    rage_DepMap * dm = rage_depmap_new();
    rage_ExtDepMap edm = rage_depmap_connect(dm, ta, tb);
    if (RAGE_FAILED(edm)) {
        rv = RAGE_FAILURE_CAST(rage_Error, edm);
    } else if (rage_depmap_outputs(dm, a) != NULL) {
        rv = RAGE_ERROR("Original depmap mutated");
    } else {
        rage_depmap_free(dm);
        dm = RAGE_SUCCESS_VALUE(edm);
        rage_ConnTerminals * ts = rage_depmap_outputs(dm, a);
        if (ts->next != NULL || ts->term.harness != b || ts->term.idx != 0) {
            rv = RAGE_ERROR("Connection not reported as output");
        } else {
            rage_conn_terms_free(ts);
            ts = rage_depmap_inputs(dm, b);
            if (ts->next != NULL || ts->term.harness != a || ts->term.idx != 0) {
                rv = RAGE_ERROR("Connection not reported as input");
            } else {
                if (RAGE_IS_JUST(rage_depmap_input_for(dm, ta))) {
                    rv = RAGE_ERROR("Input reported for unconnected port");
                } else {
                    rage_MaybeConnTerminal mct = rage_depmap_input_for(dm, tb);
                    if (!RAGE_IS_JUST(mct) || RAGE_FROM_JUST(mct).harness != a) {
                        rv = RAGE_ERROR("Input for did not report expected input");
                    }
                }
            }
        }
        rage_conn_terms_free(ts);
    }
    rage_depmap_free(dm);
    return rv;
}
