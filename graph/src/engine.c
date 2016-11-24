#include "engine.h"
#include "macros.h"
#include <stdlib.h>
#include <stdio.h>
#include <jack/jack.h>

typedef struct {
    jack_port_t * port;
    uint32_t idx;
} rage_JackPortThing;

struct rage_Harness {
    rage_Port * rage_ports;
    RAGE_ARRAY(rage_JackPortThing) jack_ports;
};

struct rage_Engine {
    jack_client_t * jack_client;
    RAGE_ARRAY(rage_Harness) harnesses;
};

static int process(jack_nframes_t nframes, void * arg) {
    rage_Engine * const e = (rage_Engine *) arg;
    for (uint32_t hid = 0; hid < e->harnesses.len; hid++) {
        rage_Harness * harness = &e->harnesses.items[hid];
        for (
                uint32_t jack_port_idx = 0;
                jack_port_idx < harness->jack_ports.len; jack_port_idx++) {
            rage_JackPortThing * jpt = &harness->jack_ports.items[jack_port_idx];
            harness->rage_ports[jpt->idx].out.samples = jack_port_get_buffer(
                jpt->port, nframes);
        }
    }
    return 0;
}

rage_NewEngine rage_engine_new() {
    rage_Engine * e = malloc(sizeof(rage_Engine));
    // FIXME: Error handling etc.
    e->jack_client = jack_client_open("rage", JackNoStartServer, NULL);
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

rage_MountResult rage_engine_mount(rage_Engine * engine, rage_Element * elem, char const * name) {
    rage_Harness * const harness = malloc(sizeof(rage_Harness));
    char * const port_name = malloc(jack_port_name_size());
    uint32_t const n_ports = rage_port_description_count(elem->ports);
    rage_Port * ports = calloc(n_ports, sizeof(rage_Port));
    uint32_t n_jack_ports = 0;
    for (rage_PortDescription const * pd = elem->ports; pd != NULL; pd = pd->next) {
        if (pd->type == RAGE_PORT_STREAM)
            n_jack_ports++;
    }
    harness->jack_ports.len = n_jack_ports;
    harness->jack_ports.items = calloc(n_jack_ports, sizeof(rage_JackPortThing));
    uint32_t jack_port_idx = 0;
    unsigned port_idx = 0;
    for (rage_PortDescription const * pd = elem->ports; pd != NULL; pd = pd->next) {
        rage_Port * port = &ports[port_idx++];
        port->desc = pd;
        switch (pd->type) {
            case (RAGE_PORT_STREAM):
                switch (pd->stream_def) {
                    case (RAGE_STREAM_AUDIO):
                        snprintf(
                            port_name, jack_port_name_size(), "%s_%u", name,
                            jack_port_idx);
                        jack_port_t * new_port = jack_port_register(
                            engine->jack_client, port_name,
                            JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
                        harness->jack_ports.items[jack_port_idx].port = new_port;
                        harness->jack_ports.items[jack_port_idx++].idx = port_idx;
                        break;
                }
                break;
            case (RAGE_PORT_EVENT):
                // FIXME: Copied from test util thingy
                // FIXME: magic const for no of events
                port->out.events = rage_event_frame_new(
                    pd->event_def.len * 8 * sizeof(rage_Atom));
                // FIXME: Hideous empty event thing below should die:
                port->out.events->events.len = 1;
                port->out.events->events.items = (rage_TimePoint *) port->out.events->buffer;
                break;
        }
    }
    free(port_name);
    harness->rage_ports = ports;
    // FIXME: proper appending
    engine->harnesses.items = harness;
    engine->harnesses.len = 1;
    RAGE_SUCCEED(rage_MountResult, harness)
}

void rage_engine_unmount(rage_Engine * engine, rage_Harness * harness) {
    // FIXME: potential races here :(
    engine->harnesses.len = 0;
    engine->harnesses.items = NULL;
    for (uint32_t jpi = 0; jpi < harness->jack_ports.len; jpi++) {
        // FIXME: ignoring error codes of this
        jack_port_unregister(engine->jack_client, harness->jack_ports.items[jpi].port);
    }
    free(harness->rage_ports);
    free(harness);
}
