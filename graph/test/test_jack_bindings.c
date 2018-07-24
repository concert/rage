#include "jack_bindings.h"

static void fake_process() {
}

static rage_Error test_jack_bindings() {
    char item_name[] = "port";
    char * iptr = item_name;
    rage_BackendDescriptions test_ports_def = {
        .len = 1,
        .items = &iptr
    };
    rage_BackendConfig bc = {
        .sample_rate = 44100,
        .buffer_size = 1024,
        .ports = {.inputs = test_ports_def, .outputs = test_ports_def},
        .process = fake_process,
        .data = NULL
    };
    rage_NewJackBackend ne = rage_jack_backend_new(bc);
    if (RAGE_FAILED(ne))
        return RAGE_FAILURE_CAST(rage_Error, ne);
    rage_JackBackend * e = RAGE_SUCCESS_VALUE(ne);
    rage_Error err = rage_jack_backend_activate(e);
    if (!RAGE_FAILED(err)) {
        err = rage_jack_backend_deactivate(e);
    }
    rage_jack_backend_free(e);
    return err;
}
