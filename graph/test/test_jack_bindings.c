#include "jack_bindings.h"

rage_Error test_jack_bindings() {
    rage_NewJackBinding ne = rage_jack_binding_new(NULL, 44100);
    RAGE_EXTRACT_VALUE(rage_Error, ne, rage_JackBinding * e)
    rage_Error err = rage_jack_binding_start(e);
    if (!RAGE_FAILED(err)) {
        err = rage_jack_binding_stop(e);
    }
    rage_jack_binding_free(e);
    return err;
}
