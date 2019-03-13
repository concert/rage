#include "jack_bindings.h"

static void noop() {
}

static rage_Error test_jack_bindings() {
    char in_name[] = "in";
    char * iptr = in_name;
    rage_BackendDescriptions in_ports_def = {
        .len = 1,
        .items = &iptr
    };
    char out_name[] = "out";
    char * optr = out_name;
    rage_BackendDescriptions out_ports_def = {
        .len = 1,
        .items = &optr
    };
    rage_BackendConfig bc = {
        .sample_rate = 44100,
        .buffer_size = 1024,
        .ports = {.inputs = in_ports_def, .outputs = out_ports_def},
    };
    rage_NewJackBackend ne = rage_jack_backend_new(bc);
    if (RAGE_FAILED(ne))
        return RAGE_FAILURE_CAST(rage_Error, ne);
    rage_JackBackend * e = RAGE_SUCCESS_VALUE(ne);
    uint32_t sr, bs;
    rage_backend_setup_process(
        rage_jack_backend_get_interface(e),
        NULL, noop, noop, &sr, &bs);
    rage_Error err = rage_jack_backend_activate(e);
    if (!RAGE_FAILED(err)) {
        err = rage_jack_backend_deactivate(e);
    }
    rage_jack_backend_free(e);
    return err;
}
