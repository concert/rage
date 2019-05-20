#include <semaphore.h>
#include <bulk_backend.h>

static void post_sem_proc(uint32_t n_frames, void * data) {
    sem_post(data);
}

static rage_Error test_bulk_backend() {
    rage_Error err = RAGE_OK;
    rage_BulkBackend * b = rage_bulk_backend_new(11, 12);
    rage_BackendInterface * i = rage_bulk_backend_get_interface(b);
    uint32_t ps;
    sem_t proc_sem;
    sem_init(&proc_sem, 0, 0);
    rage_BackendHooks h = rage_backend_setup_process(
        i, &proc_sem, post_sem_proc, noop, &ps);
    if (ps != 12) {
        err = RAGE_ERROR("Did not set period size correctly");
        goto cleanup;
    }
    err = i->activate(i->state);
    if (RAGE_FAILED(err)) {
        goto cleanup;
    }
    sem_wait(&proc_sem);
    rage_Ticking * tk = h.tick_ensure_start(h.b);
    h.tick_ensure_end(tk);
    h.get_buffers(h.b, 0, 312, NULL, NULL);
    err = i->deactivate(i->state);
    cleanup:
    rage_bulk_backend_free(b);
    return err;
}
