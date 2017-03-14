#include "jack_bindings.h"
#include "loader.h"
#include "macros.h"
#include "countdown.h"
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
    rage_JackBinding * engine;
    rage_Element * elem;
    union {
        rage_Ports;
        rage_Ports ports;
    };
    uint32_t n_inputs;
    jack_port_t ** jack_inputs;
    uint32_t n_outputs;
    jack_port_t ** jack_outputs;
    rage_Interpolator ** interpolators;
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

rage_NewJackBinding rage_jack_binding_new() {
    // FIXME: Error handling etc.
    jack_client_t * client = jack_client_open("rage", JackNoStartServer, NULL);
    if (client == NULL) {
        RAGE_FAIL(rage_NewJackBinding, "Could not create jack client")
    }
    rage_JackBinding * e = malloc(sizeof(rage_JackBinding));
    e->desired_transport = e->rt_transport = RAGE_TRANSPORT_STOPPED;
    sem_init(&e->transport_synced, 0, 0);
    e->rolling_countdown = rage_countdown_new(0);
    e->jack_client = client;
    e->harnesses.items = NULL;
    e->harnesses.len = 0;
    jack_set_process_callback(e->jack_client, process, e);
    RAGE_SUCCEED(rage_NewJackBinding, e);
}

void rage_jack_binding_free(rage_JackBinding * engine) {
    rage_countdown_free(engine->rolling_countdown);
    // FIXME: this is probably a hint that starting and stopping should be
    // elsewhere as the close has a return code
    jack_client_close(engine->jack_client);
    free(engine);
}

rage_Error rage_jack_binding_start(rage_JackBinding * engine) {
    if (jack_activate(engine->jack_client)) {
        RAGE_ERROR("Unable to activate jack client");
    }
    RAGE_OK
}

rage_Error rage_jack_binding_stop(rage_JackBinding * engine) {
    //FIXME: handle errors
    jack_deactivate(engine->jack_client);
    RAGE_OK
}

typedef RAGE_OR_ERROR(rage_Interpolator **) InterpolatorsForResult;

static InterpolatorsForResult interpolators_for(
        rage_ProcessRequirementsControls const * control_requirements,
        rage_TimeSeries * const control_values, uint8_t const n_views) {
    uint32_t const n_controls = control_requirements->len;
    rage_Interpolator ** new_interpolators = calloc(
        n_controls, sizeof(rage_Interpolator *));
    if (new_interpolators == NULL) {
        RAGE_FAIL(
            InterpolatorsForResult,
            "Unable to allocate memory for new interpolators")
    }
    for (uint32_t i = 0; i < n_controls; i++) {
        // FIXME: const sample rate
        rage_InitialisedInterpolator ii = rage_interpolator_new(
            &control_requirements->items[i], &control_values[i], 44100,
            n_views);
        if (RAGE_FAILED(ii)) {
            if (i) {
                do {
                    i--;
                    rage_interpolator_free(
                        &control_requirements->items[i], new_interpolators[i]);
                } while (i > 0);
            }
            free(new_interpolators);
            RAGE_FAIL(InterpolatorsForResult, RAGE_FAILURE_VALUE(ii));
            break;
        } else {
            new_interpolators[i] = RAGE_SUCCESS_VALUE(ii);
        }
    }
    RAGE_SUCCEED(InterpolatorsForResult, new_interpolators)
}

rage_MountResult rage_jack_binding_mount(
        rage_JackBinding * engine, rage_Element * elem, rage_TimeSeries * controls,
        char const * name) {
    rage_JackHarness * const harness = malloc(sizeof(rage_JackHarness));
    harness->engine = engine;
    harness->ports = rage_ports_new(&elem->requirements);
    size_t const name_size = jack_port_name_size();
    char * const port_name = malloc(name_size);
    harness->jack_inputs = calloc(elem->inputs.len, sizeof(jack_port_t *));
    for (uint32_t i = 0; i < elem->inputs.len; i++) {
        snprintf(port_name, name_size, "%s_in%u", name, i);
        // FIXME other stream types
        harness->jack_inputs[i] = jack_port_register(
            engine->jack_client, port_name,
            JACK_DEFAULT_AUDIO_TYPE,
            JackPortIsInput,
            0);
    }
    harness->jack_outputs = calloc(elem->outputs.len, sizeof(jack_port_t *));
    for (uint32_t i = 0; i < elem->outputs.len; i++) {
        snprintf(port_name, name_size, "%s_out%u", name, i);
        // FIXME other stream types
        harness->jack_outputs[i] = jack_port_register(
            engine->jack_client, port_name,
            JACK_DEFAULT_AUDIO_TYPE,
            JackPortIsOutput,
            0);
    }
    free(port_name);
    harness->elem = elem;
    harness->n_inputs = elem->inputs.len;
    harness->n_outputs = elem->outputs.len;
    uint8_t n_views = 1;
    if (elem->type->prep != NULL) {
        n_views++;
    }
    if (elem->type->clean != NULL) {
        n_views++;
    }
    // FIXME: Error handling
    harness->interpolators = RAGE_SUCCESS_VALUE(interpolators_for(
        &elem->controls, controls, n_views));
    for (uint32_t i = 0; i < elem->controls.len; i++) {
        uint8_t view_idx = 0;
        harness->ports.controls[i] = rage_interpolator_get_view(
            harness->interpolators[i], view_idx);
        if (elem->type->prep != NULL) {
            // FIXME: use this
            harness->ports.controls[i] = rage_interpolator_get_view(
                harness->interpolators[i], ++view_idx);
        }
        if (elem->type->clean != NULL) {
            // FIXME: use this
            harness->ports.controls[i] = rage_interpolator_get_view(
                harness->interpolators[i], ++view_idx);
        }
    }
    // FIXME: proper appending
    engine->harnesses.items = harness;
    engine->harnesses.len = 1;
    RAGE_SUCCEED(rage_MountResult, harness)
}

void rage_jack_binding_unmount(rage_JackHarness * harness) {
    // FIXME: potential races here :(
    harness->engine->harnesses.len = 0;
    harness->engine->harnesses.items = NULL;
    rage_ports_free(harness->ports);
    for (uint32_t i = 0; i < harness->n_inputs; i++) {
        // FIXME: ignoring error codes of this
        jack_port_unregister(harness->engine->jack_client, harness->jack_inputs[i]);
    }
    for (uint32_t i = 0; i < harness->n_outputs; i++) {
        // FIXME: ignoring error codes of this
        jack_port_unregister(harness->engine->jack_client, harness->jack_outputs[i]);
    }
    free(harness->jack_inputs);
    free(harness->jack_outputs);
    free(harness);
}

rage_Finaliser * rage_harness_set_time_series(
        rage_JackHarness * const harness,
        uint32_t const series_idx,
        rage_TimeSeries const * const new_controls) {
    // FIXME: fixed offset (should be derived from sample rate and period size at least)
    rage_FrameNo const offset = 4096;
    rage_FrameNo const change_at = rage_interpolated_view_get_pos(harness->ports.controls[series_idx]) + offset;
    return rage_interpolator_change_timeseries(harness->interpolators[series_idx], new_controls, change_at);
}
