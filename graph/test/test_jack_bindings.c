#include "jack_bindings.h"

static void noop() {
}

static rage_Error test_jack_bindings() {
    char in_name[] = "in";
    char * iptr = in_name;
    rage_PortNames in_ports_def = {
        .len = 1,
        .items = &iptr
    };
    char out_name[] = "out";
    char * optr = out_name;
    rage_PortNames out_ports_def = {
        .len = 1,
        .items = &optr
    };
    rage_NewJackBackend ne = rage_jack_backend_new(
        44100, 1024, in_ports_def, out_ports_def);
    if (RAGE_FAILED(ne))
        return RAGE_FAILURE_CAST(rage_Error, ne);
    rage_JackBackend * e = RAGE_SUCCESS_VALUE(ne);
    rage_BackendInterface * bi = rage_jack_backend_get_interface(e);
    uint32_t bs;
    rage_backend_setup_process(bi, NULL, noop, noop, &bs);
    rage_Error err = rage_backend_activate(bi);
    if (!RAGE_FAILED(err)) {
        err = rage_backend_deactivate(bi);
    }
    rage_jack_backend_free(e);
    return err;
}
