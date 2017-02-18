#include "harness.h"
#include "loader.h"
#include "macros.h"
#include "countdown.h"
#include <stdlib.h>
#include <stdio.h>
#include <jack/jack.h>

struct rage_Engine {
    jack_client_t * jack_client;
    RAGE_ARRAY(rage_Harness) harnesses;
    rage_TransportState transport;
};

struct rage_Harness {
    rage_Engine * engine;
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
    rage_Countdown * countdown;
};

static int process(jack_nframes_t nframes, void * arg) {
    rage_Engine * const e = (rage_Engine *) arg;
    uint32_t i;
    for (uint32_t hid = 0; hid < e->harnesses.len; hid++) {
        rage_Harness * harness = &e->harnesses.items[hid];
        for (i = 0; i < harness->n_inputs; i++) {
            harness->inputs[i] = jack_port_get_buffer(harness->jack_inputs[i], nframes);
        }
        for (i = 0; i < harness->n_outputs; i++) {
            harness->outputs[i] = jack_port_get_buffer(harness->jack_outputs[i], nframes);
        }
        rage_countdown_add(harness->countdown, -1);
        // FIXME: interpolator clock&transport sync
        rage_element_process(harness->elem, harness->engine->transport, &harness->ports);
    }
    return 0;
}

rage_NewEngine rage_engine_new() {
    // FIXME: Error handling etc.
    jack_client_t * client = jack_client_open("rage", JackNoStartServer, NULL);
    if (client == NULL) {
        RAGE_FAIL(rage_NewEngine, "Could not create jack client")
    }
    rage_Engine * e = malloc(sizeof(rage_Engine));
    e->jack_client = client;
    e->harnesses.items = NULL;
    e->harnesses.len = 0;
    jack_set_process_callback(e->jack_client, process, e);
    RAGE_SUCCEED(rage_NewEngine, e);
}

void rage_engine_free(rage_Engine * engine) {
    // FIXME: this is probably a hint that starting and stopping should be
    // elsewhere as the close has a return code
    jack_client_close(engine->jack_client);
    free(engine);
}

rage_Error rage_engine_start(rage_Engine * engine) {
    if (jack_activate(engine->jack_client)) {
        RAGE_ERROR("Unable to activate jack client");
    }
    RAGE_OK
}

rage_Error rage_engine_stop(rage_Engine * engine) {
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

rage_MountResult rage_engine_mount(
        rage_Engine * engine, rage_Element * elem, rage_TimeSeries * controls,
        char const * name) {
    rage_Harness * const harness = malloc(sizeof(rage_Harness));
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
    // Atomic control change sync stuff
    harness->countdown = rage_countdown_new(0);
    // FIXME: proper appending
    engine->harnesses.items = harness;
    engine->harnesses.len = 1;
    RAGE_SUCCEED(rage_MountResult, harness)
}

void rage_engine_unmount(rage_Harness * harness) {
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
    rage_countdown_free(harness->countdown);
    free(harness);
}

rage_Finaliser * rage_harness_set_time_series(
        rage_Harness * const harness,
        uint32_t const series_idx,
        rage_TimeSeries const * const new_controls) {
    // FIXME: fixed offset (should be derived from sample rate and period size at least)
    rage_FrameNo const offset = 4096;
    rage_FrameNo const change_at = rage_interpolated_view_get_pos(harness->ports.controls[series_idx]) + offset;
    return rage_interpolator_change_timeseries(harness->interpolators[series_idx], new_controls, change_at);
}
