#include "jack_bindings.h"

rage_Error test_jack_bindings() {
    rage_NewJackBinding ne = rage_jack_binding_new(NULL, 44100);
    if (RAGE_FAILED(ne))
        return RAGE_FAILURE_CAST(rage_Error, ne);
    rage_JackBinding * e = RAGE_SUCCESS_VALUE(ne);
    rage_Error err = rage_jack_binding_start(e);
    rage_jack_binding_transport_seek(e, 465);
    if (!RAGE_FAILED(err)) {
        err = rage_jack_binding_stop(e);
    }
    rage_jack_binding_free(e);
    return err;
}
