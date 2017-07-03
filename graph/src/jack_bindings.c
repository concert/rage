#include "jack_bindings.h"
#include "loader.h"
#include "macros.h"
#include <stdlib.h>
#include <stdio.h>
#include <jack/jack.h>
#include <semaphore.h>

struct rage_JackBinding {
    jack_client_t * jack_client;
    RAGE_ARRAY(rage_JackHarness) harnesses;
    rage_TransportState desired_transport;
    rage_TransportState rt_transport;
    sem_t transport_synced;
    rage_Countdown * rolling_countdown;
};

struct rage_JackHarness {
    rage_JackBinding * jack_binding;
    rage_Element * elem;
    union {
        rage_Ports;
        rage_Ports ports;
    };
    uint32_t n_inputs;
    jack_port_t ** jack_inputs;
    uint32_t n_outputs;
    jack_port_t ** jack_outputs;
};

static int process(jack_nframes_t nframes, void * arg) {
    rage_JackBinding * const e = (rage_JackBinding *) arg;
    uint32_t i;
    // FIXME: does the read of desired_transport need to be atomic?
    // Could use trylock if required.
    bool transport_changed = e->desired_transport != e->rt_transport;
    e->rt_transport = e->desired_transport;
    for (uint32_t hid = 0; hid < e->harnesses.len; hid++) {
        rage_JackHarness * harness = &e->harnesses.items[hid];
        for (i = 0; i < harness->n_inputs; i++) {
            harness->inputs[i] = jack_port_get_buffer(harness->jack_inputs[i], nframes);
        }
        for (i = 0; i < harness->n_outputs; i++) {
            harness->outputs[i] = jack_port_get_buffer(harness->jack_outputs[i], nframes);
        }
        // FIXME: interpolator clock&transport sync
        rage_element_process(harness->elem, e->rt_transport, &harness->ports);
    }
    switch (e->rt_transport) {
        case RAGE_TRANSPORT_ROLLING:
            rage_countdown_add(e->rolling_countdown, -1);
            break;
        case RAGE_TRANSPORT_STOPPED:
            if (transport_changed) {
                int countdown_value = rage_countdown_add(
                    e->rolling_countdown, -1);
                rage_countdown_add(e->rolling_countdown, -countdown_value);
            }
            break;
    }
    if (transport_changed) {
        sem_post(&e->transport_synced);
    }
    return 0;
}

rage_NewJackBinding rage_jack_binding_new(
        rage_Countdown * countdown, uint32_t sample_rate) {
    // FIXME: Error handling etc.
    jack_client_t * client = jack_client_open("rage", JackNoStartServer, NULL);
    if (client == NULL) {
        return RAGE_FAILURE(rage_NewJackBinding, "Could not create jack client");
    }
    uint32_t jack_sample_rate = jack_get_sample_rate(client);
    if (jack_sample_rate != sample_rate) {
        jack_client_close(client);
        return RAGE_FAILURE(rage_NewJackBinding, "Engine and jack sample rates do not match");
    }
    rage_JackBinding * e = malloc(sizeof(rage_JackBinding));
    e->desired_transport = e->rt_transport = RAGE_TRANSPORT_STOPPED;
    sem_init(&e->transport_synced, 0, 0);
    e->jack_client = client;
    e->rolling_countdown = countdown;
    e->harnesses.items = NULL;
    e->harnesses.len = 0;
    jack_set_process_callback(e->jack_client, process, e);
    return RAGE_SUCCESS(rage_NewJackBinding, e);
}

void rage_jack_binding_free(rage_JackBinding * jack_binding) {
    // FIXME: this is probably a hint that starting and stopping should be
    // elsewhere as the close has a return code
    jack_client_close(jack_binding->jack_client);
    free(jack_binding);
}

rage_Error rage_jack_binding_start(rage_JackBinding * jack_binding) {
    if (jack_activate(jack_binding->jack_client)) {
        RAGE_ERROR("Unable to activate jack client");
    }
    RAGE_OK
}

rage_Error rage_jack_binding_stop(rage_JackBinding * jack_binding) {
    if (jack_deactivate(jack_binding->jack_client)) {
        RAGE_ERROR("Jack deactivation failed")
    }
    RAGE_OK
}

rage_JackHarness * rage_jack_binding_mount(
        rage_JackBinding * jack_binding, rage_Element * elem,
        rage_InterpolatedView ** view, char const * name) {
    rage_JackHarness * const harness = malloc(sizeof(rage_JackHarness));
    harness->jack_binding = jack_binding;
    harness->ports = rage_ports_new(&elem->spec);
    size_t const name_size = jack_port_name_size();
    char * const port_name = malloc(name_size);
    harness->jack_inputs = calloc(elem->inputs.len, sizeof(jack_port_t *));
    for (uint32_t i = 0; i < elem->inputs.len; i++) {
        snprintf(port_name, name_size, "%s_in%u", name, i);
        // FIXME other stream types
        harness->jack_inputs[i] = jack_port_register(
            jack_binding->jack_client, port_name,
            JACK_DEFAULT_AUDIO_TYPE,
            JackPortIsInput,
            0);
    }
    harness->jack_outputs = calloc(elem->outputs.len, sizeof(jack_port_t *));
    for (uint32_t i = 0; i < elem->outputs.len; i++) {
        snprintf(port_name, name_size, "%s_out%u", name, i);
        // FIXME other stream types
        harness->jack_outputs[i] = jack_port_register(
            jack_binding->jack_client, port_name,
            JACK_DEFAULT_AUDIO_TYPE,
            JackPortIsOutput,
            0);
    }
    free(port_name);
    harness->elem = elem;
    harness->n_inputs = elem->inputs.len;
    harness->n_outputs = elem->outputs.len;
    harness->ports.controls = view;
    // FIXME: proper appending
    jack_binding->harnesses.items = harness;
    jack_binding->harnesses.len = 1;
    return harness;
}

void rage_jack_binding_unmount(rage_JackHarness * harness) {
    // FIXME: potential races here :(
    harness->jack_binding->harnesses.len = 0;
    harness->jack_binding->harnesses.items = NULL;
    rage_ports_free(harness->ports);
    for (uint32_t i = 0; i < harness->n_inputs; i++) {
        // FIXME: ignoring error codes of this
        jack_port_unregister(harness->jack_binding->jack_client, harness->jack_inputs[i]);
    }
    for (uint32_t i = 0; i < harness->n_outputs; i++) {
        // FIXME: ignoring error codes of this
        jack_port_unregister(harness->jack_binding->jack_client, harness->jack_outputs[i]);
    }
    free(harness->jack_inputs);
    free(harness->jack_outputs);
    free(harness);
}

void rage_jack_binding_set_transport_state(rage_JackBinding * binding, rage_TransportState state) {
    binding->desired_transport = state;
    sem_wait(&binding->transport_synced);
}
