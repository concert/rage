#include "jack_bindings.h"
#include "loader.h"
#include "macros.h"
#include <stdlib.h>
#include <stdio.h>
#include <jack/jack.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <sched.h>

typedef RAGE_ARRAY(jack_port_t *) rage_JackPorts;

_Static_assert(ATOMIC_INT_LOCK_FREE, "Period counter atomic not lock free");

struct rage_BackendState {
    jack_client_t * jack_client;
    bool active;
    volatile bool forcing_ticks;
    _Atomic unsigned period_count;
    rage_JackPorts input_ports;
    rage_JackPorts output_ports;
    rage_BackendInterface backend;
    RAGE_EMBED_STRUCT(rage_BackendConfig, conf);
};

void rage_jack_backend_get_buffers(
        rage_BackendState * state, uint32_t ext_revision,
        uint32_t n_frames, void ** inputs, void ** outputs) {
    uint32_t i;
    for (i = 0; i < state->input_ports.len; i++) {
        inputs[i] = jack_port_get_buffer(state->input_ports.items[i], n_frames);
    }
    for (i = 0; i < state->output_ports.len; i++) {
        outputs[i] = jack_port_get_buffer(state->output_ports.items[i], n_frames);
    }
}

int rage_values_eq(jack_nframes_t new_rate, void * p) {
    uint32_t * required_rate = p;
    // success code 0 if rates are equal
    return *required_rate != new_rate;
}

int rage_jack_process_cb(jack_nframes_t n_frames, void * data) {
    rage_JackBackend * jb = data;
    jb->process(&jb->backend, n_frames, jb->data);
    atomic_fetch_add_explicit(&jb->period_count, 1, memory_order_relaxed);
    return 0;
}

rage_NewJackBackend rage_jack_backend_new(rage_BackendConfig const conf) {
    jack_client_t * jc = jack_client_open("rage", JackNoStartServer, NULL);
    if (jc == NULL) {
        return RAGE_FAILURE(rage_NewJackBackend, "Unable to open jack client");
    }
    if (jack_get_sample_rate(jc) != conf.sample_rate) {
        jack_client_close(jc);  // FIXME: can fail?
        return RAGE_FAILURE(rage_NewJackBackend, "Mismatched sample rate");
    }
    rage_JackBackend * be = malloc(sizeof(rage_JackBackend));
    atomic_init(&be->period_count, 0);
    be->jack_client = jc;
    be->conf = conf;
    jack_set_sample_rate_callback(jc, rage_values_eq, &be->sample_rate);  // FIXME: can fail
    jack_set_buffer_size_callback(jc, rage_values_eq, &be->buffer_size);  //  ^
    jack_set_process_callback(jc, rage_jack_process_cb, be);
    RAGE_ARRAY_INIT(&be->input_ports, conf.ports.inputs.len, i) {
        be->input_ports.items[i] = jack_port_register(
            jc,
            conf.ports.inputs.items[i],
            JACK_DEFAULT_AUDIO_TYPE,
            JackPortIsInput,
            0);
        if (be->input_ports.items[i] == NULL) {
            be->output_ports.items = NULL;
            rage_jack_backend_free(be);
            return RAGE_FAILURE(
                rage_NewJackBackend, "Failed to create input port");
        }
    }
    RAGE_ARRAY_INIT(&be->output_ports, conf.ports.outputs.len, i) {
        be->output_ports.items[i] = jack_port_register(
            jc,
            conf.ports.outputs.items[i],
            JACK_DEFAULT_AUDIO_TYPE,
            JackPortIsOutput,
            0);
        if (be->output_ports.items[i] == NULL) {
            rage_jack_backend_free(be);
            return RAGE_FAILURE(
                rage_NewJackBackend, "Failed to create output port");
        }
    }
    be->backend.state = be;
    be->backend.get_buffers = rage_jack_backend_get_buffers;
    be->active = false;
    be->forcing_ticks = false;
    return RAGE_SUCCESS(rage_NewJackBackend, be);
}

void rage_jack_backend_free(rage_JackBackend * jbe) {
    jack_client_close(jbe->jack_client);  // FIXME: can theoretically fail
    free(jbe->input_ports.items);
    free(jbe->output_ports.items);
    free(jbe);
}

rage_Error rage_jack_backend_activate(rage_JackBackend * jbe) {
    if (jack_activate(jbe->jack_client)) {
        return RAGE_ERROR("Failed to activate");
    }
    jbe->active = true;
    return RAGE_OK;
}

rage_Error rage_jack_backend_deactivate(rage_JackBackend * jbe) {
    if (jack_deactivate(jbe->jack_client)) {
        return RAGE_ERROR("Failed to activate");
    }
    jbe->active = false;
    return RAGE_OK;
}

/*
 * Monotonic estimate of time.
 */
rage_Time rage_jack_backend_nowish(rage_JackBackend * jbe) {
    unsigned long n_frames = atomic_load_explicit(&jbe->period_count, memory_order_relaxed) * jbe->buffer_size;
    return (rage_Time) {
        .second = n_frames / jbe->sample_rate,
        .fraction = ((n_frames % jbe->sample_rate) * UINT32_MAX) / jbe->sample_rate
    };
}

#include <stdio.h>

static void * forced_ticker(void * data) {
    rage_JackBackend * jb = data;
    while (jb->forcing_ticks) {
        // Process 0 frames:
        jb->process(&jb->backend, 0, jb->data);
        // The intention of this is to unblock other threads, so give them a
        // chance to run:
        sched_yield();
    }
    return NULL;
}

struct rage_TickForcing {
    rage_JackBackend * jb;
    bool thread_started;
    pthread_t thread_id;
};

// NOTE: Not safe to have multiple of these contexts concurrently at the
// moment (or to call stop whilst in a context):
rage_TickForcing * rage_jack_backend_tick_force_start(rage_JackBackend * jbe) {
    rage_TickForcing * tf = malloc(sizeof(rage_TickForcing));
    tf->jb = jbe;
    tf->thread_started = !jbe->active;
    if (tf->thread_started) {
        jbe->forcing_ticks = true;
        // FIXME: In theory can fail:
        pthread_create(&tf->thread_id, NULL, forced_ticker, jbe);
    }
    return tf;
}

void rage_jack_backend_tick_force_end(rage_TickForcing * tf) {
    tf->jb->forcing_ticks = false;
    if (tf->thread_started) {
        pthread_join(tf->thread_id, NULL);
    }
    free(tf);
}
