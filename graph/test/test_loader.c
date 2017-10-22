#include <stdlib.h>
#include <sys/stat.h>
#include "loader.h"
#include "test_factories.h"
#include "pdls.c"

static void new_stream_buffers(rage_Ports * ports, rage_InstanceSpec spec) {
    uint32_t i;
    for (i = 0; i < spec.inputs.len; i++) {
        ports->inputs[i] = calloc(256, sizeof(float));
    }
    for (i = 0; i < spec.outputs.len; i++) {
        ports->outputs[i] = calloc(256, sizeof(float));
    }
}

static void free_stream_buffers(rage_Ports * ports, rage_InstanceSpec spec) {
    uint32_t i;
    for (i = 0; i < spec.inputs.len; i++) {
        free((void *) ports->inputs[i]);
    }
    for (i = 0; i < spec.outputs.len; i++) {
        free((void *) ports->outputs[i]);
    }
}

typedef RAGE_ARRAY(rage_Interpolator *) rage_InterpolatorArray;
typedef RAGE_OR_ERROR(rage_InterpolatorArray) rage_Interpolators;

static void free_interpolators(rage_InterpolatorArray interpolators, rage_InstanceSpec spec) {
    for (uint32_t i = 0; i < interpolators.len; i++) {
        rage_interpolator_free(&spec.controls.items[i], interpolators.items[i]);
    }
    free(interpolators.items);
}

static rage_Interpolators new_interpolators(rage_Ports * ports, rage_InstanceSpec spec) {
    rage_InterpolatorArray interpolators;
    RAGE_ARRAY_INIT(&interpolators, spec.controls.len, i) {
        rage_TimeSeries ts = rage_time_series_new(&spec.controls.items[i]);
        rage_InitialisedInterpolator ii = rage_interpolator_new(
            &spec.controls.items[i], &ts, 44100, 1);
        rage_time_series_free(ts);
        if (RAGE_FAILED(ii)) {
            interpolators.len = i;
            free_interpolators(interpolators, spec);
            return RAGE_FAILURE_CAST(rage_Interpolators, ii);
        } else {
            interpolators.items[i] = RAGE_SUCCESS_VALUE(ii);
        }
        ports->controls[i] = rage_interpolator_get_view(interpolators.items[i], 0);
    }
    return RAGE_SUCCESS(rage_Interpolators, interpolators);
}

#define RAGE_AS_ERROR(errorable) (RAGE_FAILED(errorable)) ? \
    RAGE_FAILURE_CAST(rage_Error, errorable) : RAGE_OK

static int is_plugin(const struct dirent * entry) {
    struct stat s;
    // TODO: remove this pathname match tat once in separate packages
    if (strncmp(entry->d_name, "lib", 3) != 0) {
        return false;
    }
    if (strncmp(entry->d_name, "librage", 7) == 0) {
        return false;
    }
    if (stat(entry->d_name, &s) == 0) {
        return S_ISREG(s.st_mode);
    }
    return false;
}

rage_Error test_loader() {
    rage_ElementLoader * el = rage_fussy_element_loader_new(getenv("RAGE_ELEMENTS_PATH"), is_plugin);
    rage_ElementTypes * element_type_names = rage_element_loader_list(el);
    rage_Error err = RAGE_OK;
    for (unsigned i=0; i<element_type_names->len; i++) {
        rage_ElementTypeLoadResult et_ = rage_element_loader_load(
            el, element_type_names->items[i]);
        if (!RAGE_FAILED(et_)) {
            rage_ElementType * et = RAGE_SUCCESS_VALUE(et_);
            rage_Atom ** tups = generate_tuples(et->parameters);
            // FIXME: error handling of next line
            rage_ConcreteElementType * cet = RAGE_SUCCESS_VALUE(rage_element_type_specialise(et, tups));
            rage_ElementNewResult elem_ = rage_element_new(cet, 44100, 256);
            if (!RAGE_FAILED(elem_)) {
                rage_Element * elem = RAGE_SUCCESS_VALUE(elem_);
                rage_Ports ports = rage_ports_new(&cet->spec);
                new_stream_buffers(&ports, cet->spec);
                rage_Interpolators ii = new_interpolators(&ports, cet->spec);
                if (!RAGE_FAILED(ii)) {
                    rage_InterpolatorArray interpolators = RAGE_SUCCESS_VALUE(ii);
                    rage_element_process(elem, RAGE_TRANSPORT_STOPPED, &ports);
                    rage_element_process(elem, RAGE_TRANSPORT_ROLLING, &ports);
                    free_interpolators(interpolators, cet->spec);
                } else {
                    err = RAGE_AS_ERROR(ii);
                }
                free_stream_buffers(&ports, cet->spec);
                rage_ports_free(ports);
                rage_element_free(elem);
            } else {
                err = RAGE_AS_ERROR(elem_);
            }
            rage_concrete_element_type_free(cet);
            free_tuples(et->parameters, tups);
            rage_element_loader_unload(el, et);
        } else {
            err = RAGE_AS_ERROR(et_);
        }
        if (RAGE_FAILED(err)) {
            break;
        }
    }
    rage_element_types_free(element_type_names);
    rage_element_loader_free(el);
    return err;
}

static rage_NewInstanceSpec fake_get_ports(rage_Atom ** params) {
    rage_InstanceSpec s = {.controls = {.len = 0}, .inputs = {.len = 0}, .outputs = {.len = 0}};
    return RAGE_SUCCESS(rage_NewInstanceSpec, s);
}

static void fake_free_ports(rage_InstanceSpec s) {
}

static rage_Error test_specialisation_copies_params() {
    rage_AtomDef atom_def = {.type = RAGE_ATOM_INT};
    rage_FieldDef field_def = {.type = &atom_def};
    rage_TupleDef td = {.len = 1, .items = &field_def};
    rage_ParamDefList params = {.len = 1, .items = &td};
    rage_Atom ** tups = generate_tuples(&params);
    rage_ElementType type = {
            .parameters = &params, .get_ports=fake_get_ports,
            .free_ports=fake_free_ports};
    rage_NewConcreteElementType ncet = rage_element_type_specialise(
        &type, tups);
    rage_Error rv = RAGE_OK;
    if (RAGE_FAILED(ncet)) {
        rv = RAGE_AS_ERROR(ncet);
    } else {
        rage_ConcreteElementType * cet = RAGE_SUCCESS_VALUE(ncet);
        if (cet->params == tups) {
            rv = RAGE_ERROR("parameters not copied");
        }
    }
    free_tuples(&params, tups);
    return rv;
}
