#include <bulk_backend.h>

rage_Error test_bulk_backend() {
    rage_Error err = RAGE_OK;
    rage_BulkBackend * b = rage_bulk_backend_new(11, 12);
    rage_BackendInterface * i = rage_bulk_backend_get_interface(b);
    uint32_t ps;
    rage_BackendHooks h = rage_backend_setup_process(i, NULL, noop, noop, &ps);
    if (ps != 12) {
        err = RAGE_ERROR("Did not set period size correctly");
        goto cleanup;
    }
    err = i->activate(i->state);
    if (RAGE_FAILED(err)) {
        goto cleanup;
    }
    rage_TickForcing * tf = h.tick_force_start(h.b);
    h.tick_force_end(tf);
    h.get_buffers(h.b, 0, 312, NULL, NULL);
    err = i->deactivate(i->state);
    cleanup:
    rage_bulk_backend_free(b);
    return err;
}
