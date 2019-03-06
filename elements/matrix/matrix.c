#include <stdio.h>
#include <stdlib.h>
#include "element_impl.h"
#include "interpolation.h"

#define N_CHANNELS(max_n) {\
    .type = RAGE_ATOM_INT,\
    .name = "n_channels",\
    .constraints = {.i = {\
        .min = RAGE_JUST(1),\
        .max = RAGE_JUST(max_n)\
    }}\
}

static rage_AtomDef const n_in_channels = N_CHANNELS(2048);
static rage_AtomDef const n_out_channels = N_CHANNELS(16);

static rage_FieldDef const param_fields[] = {
    // NB: limits are somewhat arbitrary, partly for name allocations' sake and
    // partly for performance
    {.name = "in_channels", .type = &n_in_channels},
    {.name = "out_channels", .type = &n_out_channels}
};

rage_TupleDef const param_tup = {
    .name = "matrix",
    .description = "Matrix mixer",
    .default_value = NULL,
    .len = 2,
    .items = param_fields
};

rage_ParamDefList const init_params = {
    .len = 1,
    .items = &param_tup
};

static rage_AtomDef const gain_float = {
    .type = RAGE_ATOM_FLOAT, .name = "gain",
    .constraints = {.f = {}}
};

static rage_FieldDef const gain_fields[] = {
    {.name = "gain", .type = &gain_float}
};

static rage_Atom const gain_default[] = {
    {.f = 0.0}
};


static void gain_def(rage_TupleDef * const td, unsigned i, unsigned o) {
    char * name = malloc(10), * description = malloc(50);
    sprintf(name, "%i_%i", i, o);
    sprintf(description, "The amount of input %i to appear in output %i", i, o);
    td->name = name;
    td->description = description;
    td->default_value = gain_default;
    td->len = 1;
    td->items = gain_fields;
};

rage_NewInstanceSpec elem_describe_ports(rage_Atom ** params) {
    int const num_in_channels = params[0][0].i;
    int const num_out_channels = params[0][1].i;

    rage_InstanceSpec spec;
    spec.max_uncleaned_frames = UINT32_MAX;
    spec.max_period_size = UINT32_MAX;
    spec.controls.len = num_out_channels * num_in_channels;
    rage_TupleDef * items = calloc(
        num_out_channels * num_in_channels, sizeof(rage_TupleDef));
    spec.controls.items = items;
    for (unsigned o = 0; o < num_out_channels; o++) {
        for (unsigned i = 0; i < num_in_channels; i++) {
            gain_def(&items[(o * num_in_channels) + i], i, o);
        }
    }
    int const num_streams = num_in_channels + num_out_channels;
    rage_StreamDef * stream_defs = calloc(num_streams, sizeof(rage_StreamDef));
    for (unsigned i = 0; i < num_streams; i++) {
        stream_defs[i] = RAGE_STREAM_AUDIO;
    }
    spec.inputs.len = num_in_channels;
    spec.inputs.items = stream_defs;
    spec.outputs.len = num_out_channels;
    spec.outputs.items = stream_defs + (
        sizeof(rage_StreamDef) * num_in_channels);

    return RAGE_SUCCESS(rage_NewInstanceSpec, spec);
};

void elem_free_port_description(rage_InstanceSpec spec) {
    free((void *) spec.inputs.items);
    for (unsigned i = 0; i < spec.controls.len; i++) {
        free((void *) spec.controls.items[i].name);
        free((void *) spec.controls.items[i].description);
    }
    free((void *) spec.controls.items);
};

struct rage_ElementState {
    unsigned num_in_channels;
    unsigned num_out_channels;
};

rage_NewElementState elem_new(uint32_t sample_rate, rage_Atom ** params) {
    // NB: if we had pre-compiled optimisation for different numbers of common
    // inputs and outputs of the mixer, we could make that decision here.
    rage_ElementState * state = malloc(sizeof(rage_ElementState));
    state->num_in_channels = params[0][0].i;
    state->num_out_channels = params[0][1].i;
    return RAGE_SUCCESS(rage_NewElementState, state);
}

void elem_free(rage_ElementState * state) {
    free(state);
}

void elem_process(
        rage_ElementState * state,
        rage_TransportState const transport_state,
        uint32_t period_size,
        rage_Ports const * ports) {
    // NB: we could inspect period size to determine if we wanted to do some
    // snazzy optimisation here. If we were running for a long time, it would be
    // worth determining up front the matrix elements that remained zero over
    // the whole period.
    rage_InterpolatedValue const * val;
    for (unsigned o = 0; o < state->num_out_channels; o++) {
      memset(ports->outputs[o], 0, period_size * sizeof(float));
      for (unsigned i = 0; i < state->num_in_channels; i++) {
            uint32_t pos = 0, remaining = period_size;
            unsigned control_idx = (o * state->num_in_channels) + i;
            do {
                val = rage_interpolated_view_value(
                    ports->controls[control_idx]);
                // Question: would it be advantageous to make this logic around
                // what to do with interpolating control data when the transport
                // is stopped vs rolling in a more universal way?
                uint32_t n_to_change;
                switch (transport_state) {
                    case RAGE_TRANSPORT_STOPPED:
                        n_to_change = period_size;
                        break;
                    case RAGE_TRANSPORT_ROLLING:
                        n_to_change = (remaining < val->valid_for) ?
                            remaining : val->valid_for;
                        rage_interpolated_view_advance(
                            ports->controls[control_idx], n_to_change);
                }

                for (unsigned samp = pos; samp < pos + n_to_change; samp++) {
                    ports->outputs[o][samp] +=
                        ports->inputs[i][samp] * val->value[0].f;
                }

                pos += n_to_change;
                remaining -= n_to_change;
            } while (remaining);
        }
    }
}

rage_ElementKind const elem_info = {
    .parameters = &init_params,
    .state_new = elem_new,
    .state_free = elem_free,
    .ports_get = elem_describe_ports,
    .ports_free = elem_free_port_description,
    .process = elem_process
};
