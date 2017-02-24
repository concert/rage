#include "harness.h"

rage_Error test_harness() {
    rage_NewEngine ne = rage_engine_new();
    RAGE_EXTRACT_VALUE(rage_Error, ne, rage_Engine * e)
    rage_Error err = rage_engine_start(e);
    if (!RAGE_FAILED(err)) {
        err = rage_engine_stop(e);
    }
    rage_engine_free(e);
    return err;
}
