#include "engine.h"

rage_Error test_engine() {
    rage_NewEngine ne = rage_engine_new();
    rage_Engine * e;
    RAGE_EXTRACT_VALUE(rage_Error, ne, e)
    rage_engine_free(e);
    RAGE_OK
}
