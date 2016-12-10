#include "harness.h"

rage_Error test_harness() {
    rage_NewEngine ne = rage_engine_new();
    RAGE_EXTRACT_VALUE(rage_Error, ne, rage_Engine * e)
    rage_engine_free(e);
    RAGE_OK
}
