#include <stdlib.h>
#include <sys/stat.h>
#include "loader.h"
#include "test_factories.h"
#include "graph_test_factories.h"

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
            &spec.controls.items[i], &ts, 44100, 1, NULL);
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
    rage_ElementKinds * element_type_names = rage_element_loader_list(el);
    rage_Error err = RAGE_OK;
    for (unsigned i=0; i<element_type_names->len; i++) {
        rage_LoadedElementKindLoadResult ek_ = rage_element_loader_load(
            element_type_names->items[i]);
        if (!RAGE_FAILED(ek_)) {
            rage_LoadedElementKind * ek = RAGE_SUCCESS_VALUE(ek_);
            rage_Atom ** tups = generate_tuples(rage_element_kind_parameters(ek));
            rage_NewElementType cet_ = rage_element_kind_specialise(ek, tups);
            if (!RAGE_FAILED(cet_)) {
                rage_ElementType * cet = RAGE_SUCCESS_VALUE(cet_);
                if (cet->params == tups) {
                    err = RAGE_ERROR("Parameters not copied");
                }
                free_tuples(rage_element_kind_parameters(ek), tups);
                rage_ElementNewResult elem_ = rage_element_new(cet, 44100);
                if (!RAGE_FAILED(elem_)) {
                    rage_Element * elem = RAGE_SUCCESS_VALUE(elem_);
                    rage_Ports ports = rage_ports_new(&cet->spec);
                    new_stream_buffers(&ports, cet->spec);
                    rage_Interpolators ii = new_interpolators(&ports, cet->spec);
                    if (!RAGE_FAILED(ii)) {
                        rage_InterpolatorArray interpolators = RAGE_SUCCESS_VALUE(ii);
                        rage_element_process(elem, RAGE_TRANSPORT_STOPPED, 256, &ports);
                        rage_element_process(elem, RAGE_TRANSPORT_ROLLING, 256, &ports);
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
                rage_element_type_free(cet);
            } else {
                err = RAGE_AS_ERROR(cet_);
            }
            rage_element_loader_unload(ek);
        } else {
            err = RAGE_AS_ERROR(ek_);
        }
        if (RAGE_FAILED(err)) {
            break;
        }
    }
    rage_element_kinds_free(element_type_names);
    rage_element_loader_free(el);
    return err;
}
