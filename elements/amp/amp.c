#include <stdlib.h>
// FIXME: these includes should be "systemy"
#include "element.h"
#include "interpolation.h"
// TODO: Make a header with the things you need to define in it and include here
// This header should probably have a hash of the interface files or something dynamically embedded in it

static rage_AtomDef const n_channels = {
    .type = RAGE_ATOM_INT,
    .name = "n_channels",
    .constraints = {.i = {.min = RAGE_JUST(1)}}
};

static rage_FieldDef const param_fields[] = {
    {.name = "channels", .type = &n_channels}
};

rage_TupleDef const init_params = {
    .name = "amp",
    .description = "Amplifier",
    .liberty = RAGE_LIBERTY_MUST_PROVIDE,
    .len = 1,
    .items = param_fields
};

static rage_AtomDef const gain_float = {
    .type = RAGE_ATOM_FLOAT, .name = "gain",
    .constraints = {.f = {.max = RAGE_JUST(5.0)}}
};

static rage_FieldDef const gain_fields[] = {
    {.name = "gain", .type = &gain_float}
};

static rage_TupleDef const gain_def = {
    .name = "gain",
    .description = "Loudness",
    .liberty = RAGE_LIBERTY_MAY_PROVIDE,
    .len = 1,
    .items = gain_fields
};

rage_PortDescription * elem_describe_ports(rage_Tuple params) {
    rage_PortDescription * prev = NULL;
    for (unsigned i=0; i<params[0].i; i++) {
        // Too implicit that you get an audio output?
        // Should there be an enum for port direction, so one can go in both?
        prev = rage_port_description_copy((rage_PortDescription) {
            .is_input = true,
            .next = prev
        });
        prev = rage_port_description_copy((rage_PortDescription) {
            .next = prev
        });
    }
    return rage_port_description_copy((rage_PortDescription) {
        .is_input = true,
        .type = RAGE_PORT_EVENT,
        .event_def = gain_def,
        .next = prev
    });
}

typedef struct {
    unsigned n_channels;
    rage_Interpolator interpolator;
    rage_Time sample_length; // Good idea?
} amp_data;

rage_NewElementState elem_new(rage_Tuple params) {
    amp_data * ad = malloc(sizeof(amp_data));
    // Not sure I like way the indices tie up here
    ad->n_channels = params[0].i;
    rage_InitialisedInterpolator interpolator = rage_interpolator_new(gain_def);
    RAGE_EXTRACT_VALUE(rage_NewElementState, interpolator, ad->interpolator)
    ad->sample_length = (rage_Time) {};  // FIXME: sample rate!
    RAGE_SUCCEED(rage_NewElementState, (void *) ad);
}

void elem_free(void * state) {
    free(state);
}


rage_Error elem_process(
        void * state, rage_Time time, unsigned nsamples, rage_Port * ports) {
    amp_data const * const data = (amp_data *) state;
    for (unsigned s = 0; s < nsamples; s++) {
        rage_Tuple gain = rage_interpolate(
            data->interpolator, time, ports[0].in.events->events);
        for (unsigned c = 1; c < data->n_channels; c += 2) {
            ports[c+1].out.samples[s] = gain[0].f * ports[c].in.samples[s];
        }
        time = rage_time_add(time, data->sample_length);
    }
    RAGE_OK
}
