#include <stdlib.h>
// FIXME: these includes should be "systemy"
// This header should probably have a hash of the interface files or something
// dynamically embedded in it:
#include "element_impl.h"
#include "interpolation.h"

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

rage_ProcessRequirements elem_describe_ports(rage_Tuple params) {
    rage_ProcessRequirements rval;
    rval.controls.len = 1;
    rval.controls.items = &gain_def;
    rage_StreamDef * stream_defs = calloc(params[0].i, sizeof(rage_StreamDef));
    for (unsigned i = 0; i < params[0].i; i++) {
        stream_defs[i] = RAGE_STREAM_AUDIO;
    }
    rval.inputs.len = params[0].i;
    rval.inputs.items = stream_defs;
    rval.outputs.len = params[0].i;
    rval.outputs.items = stream_defs;
    return rval;
}

void elem_free_port_description(rage_ProcessRequirements pr) {
    // FIXME: too const requiring cast?
    free((void *) pr.inputs.items);
}

typedef struct {
    unsigned n_channels;
    rage_Interpolator interpolator;
    rage_Time sample_length; // Good idea?
    uint32_t frame_size; // Doesn't everyone HAVE to do this?
} amp_data;

rage_NewElementState elem_new(
        uint32_t sample_rate, uint32_t frame_size, rage_Tuple params) {
    amp_data * ad = malloc(sizeof(amp_data));
    // Not sure I like way the indices tie up here
    ad->n_channels = params[0].i;
    rage_InitialisedInterpolator interpolator = rage_interpolator_new(
        gain_def);
    RAGE_EXTRACT_VALUE(rage_NewElementState, interpolator, ad->interpolator)
    ad->sample_length = rage_time_sample_length(sample_rate);
    ad->frame_size = frame_size;
    RAGE_SUCCEED(rage_NewElementState, (void *) ad);
}

void elem_free(void * state) {
    free(state);
}

rage_Error elem_process(
        void * state, rage_Time time, rage_Ports const * ports) {
    amp_data const * const data = (amp_data *) state;
    for (unsigned s = 0; s < data->frame_size; s++) {
        rage_Tuple gain = rage_interpolate(
            data->interpolator, time, ports->controls.items[0]);
        for (unsigned c = 0; c < data->n_channels; c++) {
            ports->outputs[c][s] = ports->inputs[c][s] * gain[0].f;
        }
        time = rage_time_add(time, data->sample_length);
    }
    RAGE_OK
}
