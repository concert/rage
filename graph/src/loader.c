#include <stdlib.h>
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>
#include "loader.h"

struct rage_ElementLoader {
    char * elems_path;
    rage_PluginFilter pf;
};

struct rage_LoadedElementKind {
    void * dlhandle;
    rage_ElementType * type;
};

static char * join_path(char const * a, char const * b) {
    char * joined = malloc(strlen(a) + strlen(b) + 2);
    strncpy(joined, a, strlen(a));
    joined[strlen(a)] = '/';
    strcpy(joined + strlen(a) + 1, b);
    return joined;
}

// This is pretty horrendous, but as the filter function of scandir doesn't
// take user data this is pretty much all you can do. This makes the scanning
// not threadsafe.
static char const * in_progress_scan_dir;

// Just enough to avoid getting '.' and '..' entries.
static int is_plugin(const struct dirent * entry) {
    struct stat s;
    bool r = false;
    char * entry_path = join_path(in_progress_scan_dir, entry->d_name);
    if (stat(entry_path, &s) == 0) {
        r = S_ISREG(s.st_mode);
    }
    free(entry_path);
    return r;
}

rage_ElementLoader * rage_element_loader_new(char const * elems_path) {
    return rage_fussy_element_loader_new(elems_path, is_plugin);
}

rage_ElementLoader * rage_fussy_element_loader_new(
        char const * elems_path, rage_PluginFilter pf) {
    rage_ElementLoader * el = malloc(sizeof(rage_ElementLoader));
    el->elems_path = strdup(elems_path);
    el->pf = pf;
    return el;
}

void rage_element_loader_free(rage_ElementLoader * el) {
    free(el->elems_path);
    free(el);
}

rage_ElementTypes * rage_element_loader_list(rage_ElementLoader * el) {
    rage_ElementTypes * elems = malloc(sizeof(rage_ElementTypes));
    elems->len = 0;
    elems->items = NULL;
    struct dirent ** entries;
    in_progress_scan_dir = el->elems_path;  // Set context for scan filter *ick*
    int n_items = scandir(el->elems_path, &entries, el->pf, NULL);
    if (n_items >= 0) {
        RAGE_ARRAY_INIT(elems, n_items, i) {
            elems->items[i] = join_path(el->elems_path, entries[i]->d_name);
            free(entries[i]);
        }
        free(entries);
    } else {
        // FIXME: Do something on this failure!
    }
    return elems;
}

void rage_element_types_free(rage_ElementTypes * t) {
    for (uint32_t i = 0; i < t->len; i++) {
        free(t->items[i]);
    }
    RAGE_ARRAY_FREE(t)
}

rage_LoadedElementKindLoadResult rage_element_loader_load(char const * type_name) {
    void * handle = dlopen(type_name, RTLD_LAZY);
    if (handle == NULL)
        return RAGE_FAILURE(rage_LoadedElementKindLoadResult, dlerror());
    rage_ElementType * type = dlsym(handle, "elem_info");
    #define RAGE_ETL_BAIL(msg) \
        dlclose(handle); \
        return RAGE_FAILURE(rage_LoadedElementKindLoadResult, msg);
    if (type == NULL) {
        RAGE_ETL_BAIL("Missing entry point symbol: elem_info")
    }
    #define RAGE_ETL_MANDATORY_PARAM(struct_name) \
        if (type->struct_name == NULL) {\
            RAGE_ETL_BAIL("Missing mandatory member: " #struct_name) \
        }
    RAGE_ETL_MANDATORY_PARAM(parameters)
    RAGE_ETL_MANDATORY_PARAM(state_new)
    RAGE_ETL_MANDATORY_PARAM(state_free)
    RAGE_ETL_MANDATORY_PARAM(ports_get)
    RAGE_ETL_MANDATORY_PARAM(ports_free)
    RAGE_ETL_MANDATORY_PARAM(process)
    #undef RAGE_ETL_MANDATORY_PARAM
    if (type->prep != NULL && type->clear == NULL) {
        RAGE_ETL_BAIL("Prep provide but no clear")
    }
    #undef RAGE_ETL_BAIL
    rage_LoadedElementKind * kind = malloc(sizeof(rage_LoadedElementKind));
    kind->dlhandle = handle;
    kind->type = type;
    return RAGE_SUCCESS(rage_LoadedElementKindLoadResult, kind);
}

void rage_element_loader_unload(rage_LoadedElementKind * ek) {
    dlclose(ek->dlhandle);
    free(ek);
}

rage_ParamDefList const * rage_element_kind_parameters(rage_LoadedElementKind const * kind) {
    return kind->type->parameters;
}

// ConcreteElementType

static rage_Atom ** rage_params_copy(rage_ParamDefList const * pds, rage_Atom ** params) {
    rage_Atom ** rp = calloc(pds->len, sizeof(rage_ParamDefList));
    for (uint32_t i = 0; i < pds->len; i++) {
        rp[i] = rage_tuple_copy(&pds->items[i], params[i]);
    }
    return rp;
}

static void rage_params_free(rage_ParamDefList const * pds, rage_Atom ** params) {
    for (uint32_t i = 0; i < pds->len; i++) {
        rage_tuple_free(&pds->items[i], params[i]);
    }
    free(params);
}

rage_NewConcreteElementType rage_element_type_specialise(
        rage_LoadedElementKind * kind, rage_Atom ** params) {
    rage_NewInstanceSpec new_ports = kind->type->ports_get(params);
    if (RAGE_FAILED(new_ports))
        return RAGE_FAILURE_CAST(rage_NewConcreteElementType, new_ports);
    rage_InstanceSpec spec = RAGE_SUCCESS_VALUE(new_ports);
    if (spec.max_uncleaned_frames == 0)
        return RAGE_FAILURE(rage_NewConcreteElementType, "max_uncleaned_frames == 0");
    rage_ConcreteElementType * cet = malloc(sizeof(rage_ConcreteElementType));
    cet->type = kind->type;
    cet->params = rage_params_copy(rage_element_kind_parameters(kind), params);
    cet->spec = spec;
    return RAGE_SUCCESS(rage_NewConcreteElementType, cet);
}

void rage_concrete_element_type_free(rage_ConcreteElementType * cet) {
    cet->type->ports_free(cet->spec);
    rage_params_free(cet->type->parameters, cet->params);
    free(cet);
}

// Element

rage_ElementNewResult rage_element_new(
        rage_ConcreteElementType * cet, uint32_t sample_rate) {
    rage_NewElementState new_state = cet->type->state_new(
        sample_rate, cet->params);
    if (RAGE_FAILED(new_state))
        return RAGE_FAILURE_CAST(rage_ElementNewResult, new_state);
    rage_Element * const elem = malloc(sizeof(rage_Element));
    elem->cet = cet;
    elem->state = RAGE_SUCCESS_VALUE(new_state);
    return RAGE_SUCCESS(rage_ElementNewResult, elem);
}

void rage_element_free(rage_Element * elem) {
    elem->cet->type->state_free(elem->state);
    free(elem);
}

void rage_element_process(
        rage_Element const * const elem,
        rage_TransportState const transport_state, uint32_t period_size,
        rage_Ports const * ports) {
    elem->cet->type->process(elem->state, transport_state, period_size, ports);
}
