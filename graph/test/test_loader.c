#include <stdlib.h>
#include "loader.h"

static void new_stream_buffers(rage_Ports * ports, rage_ProcessRequirements requirements) {
    uint32_t i;
    for (i = 0; i < requirements.inputs.len; i++) {
        ports->inputs[i] = calloc(256, sizeof(float));
    }
    for (i = 0; i < requirements.outputs.len; i++) {
        ports->outputs[i] = calloc(256, sizeof(float));
    }
}

static void free_stream_buffers(rage_Ports * ports, rage_ProcessRequirements requirements) {
    uint32_t i;
    for (i = 0; i < requirements.inputs.len; i++) {
        free((void *) ports->inputs[i]);
    }
    for (i = 0; i < requirements.outputs.len; i++) {
        free((void *) ports->outputs[i]);
    }
}

static rage_Error new_interpolators(rage_Ports * ports, rage_ProcessRequirements requirements) {
    for (uint32_t i = 0; i < requirements.controls.len; i++) {
        rage_TimeSeries ts = rage_time_series_new(&requirements.controls.items[i]);
        rage_InitialisedInterpolator ii = rage_interpolator_new(
            &requirements.controls.items[i], &ts, 44100);
        rage_time_series_free(ts);
        RAGE_EXTRACT_VALUE(rage_Error, ii, ports->controls[i])
    }
    RAGE_OK
}

static void free_interpolators(rage_Ports * ports, rage_ProcessRequirements requirements) {
    for (uint32_t i = 0; i < requirements.controls.len; i++) {
        rage_interpolator_free(&requirements.controls.items[i], ports->controls[i]);
    }
}

rage_Error test() {
    // FIXME: Error handling (and memory in those cases)
    rage_ElementLoader el = rage_element_loader_new();
    rage_ElementTypes element_type_names = rage_element_loader_list(el);
    for (unsigned i=0; i<element_type_names.len; i++) {
        rage_ElementTypeLoadResult et_ = rage_element_loader_load(
            el, element_type_names.items[i]);
        RAGE_EXTRACT_VALUE(rage_Error, et_, rage_ElementType * et)
        rage_Atom * tup = rage_tuple_generate(et->parameters);
        rage_ElementNewResult elem_ = rage_element_new(et, 44100, 256, tup);
        RAGE_EXTRACT_VALUE(rage_Error, elem_, rage_Element * elem)
        rage_Ports ports = rage_ports_new(&elem->requirements);
        new_stream_buffers(&ports, elem->requirements);
        new_interpolators(&ports, elem->requirements);
        rage_element_process(elem, &ports);
        free_interpolators(&ports, elem->requirements);
        free_stream_buffers(&ports, elem->requirements);
        rage_ports_free(ports);
        rage_element_free(elem);
        free(tup);
        rage_element_loader_unload(el, et);
    }
    rage_element_loader_free(el);
    RAGE_OK
}
