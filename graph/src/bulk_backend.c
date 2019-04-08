#include "bulk_backend.h"
#include <pthread.h>
#include <assert.h>
#include <unistd.h>

struct rage_BackendState {
    uint32_t period_size;
    uint32_t sample_rate;
    volatile unsigned n_activations;
    rage_ProcessCb process;
    void * data;
    pthread_t thread_id;
    rage_BackendInterface interface;
};

static void * proc_ticker(void * data) {
    rage_BulkBackend * b = data;
    while (b->n_activations) {
        b->process(b->period_size, b->data);
        // FIXME: This is an horrific fudge to stave off having to rework the
        // threading model in the very short term
        usleep(10000);
    }
    return NULL;
}

static rage_Error activate(rage_BulkBackend * b) {
    if (b->process == NULL) {
        return RAGE_ERROR("Activating with process unset");
    }
    if (b->n_activations++ == 0) {
        if (pthread_create(&b->thread_id, NULL, proc_ticker, b)) {
            return RAGE_ERROR("Process thread failed to start");
        }
    }
    return RAGE_OK;
}

static rage_Error deactivate(rage_BulkBackend * b) {
    if (--b->n_activations == 0) {
        if (pthread_join(b->thread_id, NULL)) {
            return RAGE_ERROR("Process thread failed to stop");
        }
    }
    return RAGE_OK;
}

static rage_TickForcing * tick_force_start(rage_BulkBackend * b) {
    // FIXME: Ignoring failures!
    activate(b);
    return (rage_TickForcing *) b;
}

static void tick_force_end(rage_TickForcing * tf) {
    // FIXME: Ignoring failures!
    deactivate((rage_BulkBackend *) tf);
}

static void get_buffers() {
}

static rage_BackendHooks setup_process(
        rage_BulkBackend * be, void * data, rage_ProcessCb process,
        rage_SetExternalsCb set_externals, uint32_t * buffer_size) {
    be->process = process;
    be->data = data;
    *buffer_size = be->period_size;
    return (rage_BackendHooks) {
        .tick_force_start = tick_force_start,
        .tick_force_end = tick_force_end,
        .get_buffers = get_buffers,
        .b = be
    };
}

static void unset_process(rage_BulkBackend * be) {
    assert(!be->n_activations);
    be->process = NULL;
    be->data = NULL;
}

rage_BulkBackend * rage_bulk_backend_new(
        uint32_t const sample_rate, uint32_t const period_size) {
    rage_BulkBackend * be = malloc(sizeof(rage_BulkBackend));
    be->n_activations = 0;
    be->sample_rate = sample_rate;
    be->period_size = period_size;
    be->interface.state = be;
    be->interface.setup_process = setup_process;
    be->interface.unset_process = unset_process;
    be->interface.activate = activate;
    be->interface.deactivate = deactivate;
    be->interface.sample_rate = &be->sample_rate;
    return be;
}

rage_BackendInterface * rage_bulk_backend_get_interface(rage_BulkBackend * bb) {
    return &bb->interface;
}

void rage_bulk_backend_free(rage_BulkBackend * bb) {
    assert(!bb->n_activations);
    free(bb);
}
