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

rage_TupleDef const param_tup = {
    .name = "amp",
    .description = "Amplifier",
    .default_value = NULL,
    .len = 1,
    .items = param_fields
};

rage_ParamDefList const init_params = {
    .len = 1,
    .items = &param_tup
};

static rage_AtomDef const gain_float = {
    .type = RAGE_ATOM_FLOAT, .name = "gain",
    .constraints = {.f = {.max = RAGE_JUST(5.0)}}
};

static rage_FieldDef const gain_fields[] = {
    {.name = "gain", .type = &gain_float}
};

static rage_Atom const gain_default[] = {
    {.f = 1.0}
};

static rage_TupleDef const gain_def = {
    .name = "gain",
    .description = "Loudness",
    .default_value = gain_default,
    .len = 1,
    .items = gain_fields
};

struct rage_ElementState {
    unsigned n_channels;
};

struct rage_ElementTypeState {
    rage_ElementState;
};

rage_NewElementState elem_new(rage_ElementTypeState * type_state, uint32_t sample_rate) {
    return RAGE_SUCCESS(rage_NewElementState, (rage_ElementState *) type_state);
}

void elem_free(rage_ElementState * state) {
}

void elem_process(
        rage_ElementState * state, rage_TransportState const transport_state, uint32_t period_size, rage_Ports const * ports) {
    rage_InterpolatedValue const * val = rage_interpolated_view_value(ports->controls[0]);
    uint32_t pos = 0, remaining = period_size;
    do {
        val = rage_interpolated_view_value(ports->controls[0]);
        uint32_t n_to_change;
        switch (transport_state) {
            case RAGE_TRANSPORT_STOPPED:
                n_to_change = period_size;
                break;
            case RAGE_TRANSPORT_ROLLING:
                n_to_change = (remaining < val->valid_for) ?
                    remaining : val->valid_for;
                rage_interpolated_view_advance(
                    ports->controls[0], n_to_change);
                break;
        }
        for (unsigned s = pos; s < pos + n_to_change; s++) {
            for (unsigned c = 0; c < state->n_channels; c++) {
                ports->outputs[c][s] = ports->inputs[c][s] * val->value[0].f;
            }
        }
        pos += n_to_change;
        remaining -= n_to_change;
    } while (remaining);
}

void type_destroy(rage_ElementType * type) {
    // FIXME: too const requiring cast?
    free((void *) type->spec.inputs.items);
    free(type->type_state);
}

rage_Error kind_specialise(
        rage_ElementType * type, rage_Atom ** params) {
    int const n_channels = params[0][0].i;
    type->spec.max_uncleaned_frames = UINT32_MAX;
    type->spec.max_period_size = UINT32_MAX;
    type->spec.controls.len = 1;
    type->spec.controls.items = &gain_def;
    rage_StreamDef * stream_defs = calloc(n_channels, sizeof(rage_StreamDef));
    for (unsigned i = 0; i < n_channels; i++) {
        stream_defs[i] = RAGE_STREAM_AUDIO;
    }
    type->spec.inputs.len = n_channels;
    type->spec.inputs.items = stream_defs;
    type->spec.outputs.len = n_channels;
    type->spec.outputs.items = stream_defs;
    type->type_destroy = type_destroy;
    type->state_new = elem_new;
    type->process = elem_process;
    type->state_free = elem_free;
    type->type_state = malloc(sizeof(rage_ElementTypeState));
    type->type_state->n_channels = n_channels;
    return RAGE_OK;
}

rage_ElementKind const kind = {
    .parameters = &init_params,
    .specialise = kind_specialise
};
