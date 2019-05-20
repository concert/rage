#include <stdlib.h>
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>
#include "loader.h"
#include "elem_impl_hash.h"

struct rage_ElementLoader {
    char * elems_path;
    rage_PluginFilter pf;
};

struct rage_LoadedElementKind {
    void * dlhandle;
    rage_ElementKind * kind;
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

rage_ElementKinds * rage_element_loader_list(rage_ElementLoader * el) {
    rage_ElementKinds * elems = malloc(sizeof(rage_ElementKinds));
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

void rage_element_kinds_free(rage_ElementKinds * t) {
    for (uint32_t i = 0; i < t->len; i++) {
        free(t->items[i]);
    }
    RAGE_ARRAY_FREE(t)
}

rage_LoadedElementKindLoadResult rage_element_loader_load(char const * kind_name) {
    void * handle = dlopen(kind_name, RTLD_LAZY);
    if (handle == NULL)
        return RAGE_FAILURE(rage_LoadedElementKindLoadResult, dlerror());
    #define RAGE_ETL_BAIL(msg) \
        dlclose(handle); \
        return RAGE_FAILURE(rage_LoadedElementKindLoadResult, msg);
    uint64_t * interface_version = dlsym(handle, "rage_element_interface_hash");
    if (*interface_version != rage_element_interface_hash) {
        RAGE_ETL_BAIL("Version of loader and plugin interface mismatch")
    }
    rage_ElementKind * kind = dlsym(handle, "kind");
    if (kind == NULL) {
        RAGE_ETL_BAIL("Missing entry point symbol: kind")
    }
    #define RAGE_ETL_MANDATORY_PARAM(struct_name) \
        if (kind->struct_name == NULL) {\
            RAGE_ETL_BAIL("Missing mandatory member: " #struct_name) \
        }
    RAGE_ETL_MANDATORY_PARAM(parameters)
    RAGE_ETL_MANDATORY_PARAM(specialise)
    #undef RAGE_ETL_MANDATORY_PARAM
    #undef RAGE_ETL_BAIL
    rage_LoadedElementKind * loaded_kind = malloc(sizeof(rage_LoadedElementKind));
    loaded_kind->dlhandle = handle;
    loaded_kind->kind = kind;
    return RAGE_SUCCESS(rage_LoadedElementKindLoadResult, loaded_kind);
}

void rage_element_loader_unload(rage_LoadedElementKind * ek) {
    dlclose(ek->dlhandle);
    free(ek);
}

rage_ParamDefList const * rage_element_kind_parameters(rage_LoadedElementKind const * kind) {
    return kind->kind->parameters;
}

// ElementType

rage_NewElementType rage_element_kind_specialise(
        rage_LoadedElementKind * loaded_kind, rage_Atom ** params) {
    rage_ElementType * type = calloc(1, sizeof(rage_ElementType));
    rage_Error err = loaded_kind->kind->specialise(type, params);
    if (RAGE_FAILED(err)) {
        free(type);
        return RAGE_FAILURE_CAST(rage_NewElementType, err);
    }
    if (type->type_destroy == NULL) {
        free(type);
        return RAGE_FAILURE(rage_NewElementType, "type_destroy uninitialised");
    }
    #define RAGE_EKS_BAIL_IF(condition) \
        if (condition) { \
            rage_element_type_free(type); \
            return RAGE_FAILURE(rage_NewElementType, #condition); \
        }
    RAGE_EKS_BAIL_IF(type->state_new == NULL)
    RAGE_EKS_BAIL_IF(type->process == NULL)
    RAGE_EKS_BAIL_IF(type->state_free == NULL)
    RAGE_EKS_BAIL_IF(type->spec.max_uncleaned_frames == 0)
    RAGE_EKS_BAIL_IF(type->prep && !type->clear)
    #undef RAGE_EKS_BAIL_IF
    return RAGE_SUCCESS(rage_NewElementType, type);
}

void rage_element_type_free(rage_ElementType * et) {
    et->type_destroy(et);
    free(et);
}

// Element

rage_ElementNewResult rage_element_new(
        rage_ElementType * type, uint32_t sample_rate) {
    rage_NewElementState new_state = type->state_new(
        type->type_state, sample_rate);
    if (RAGE_FAILED(new_state))
        return RAGE_FAILURE_CAST(rage_ElementNewResult, new_state);
    rage_Element * const elem = malloc(sizeof(rage_Element));
    elem->type = type;
    elem->state = RAGE_SUCCESS_VALUE(new_state);
    return RAGE_SUCCESS(rage_ElementNewResult, elem);
}

void rage_element_free(rage_Element * elem) {
    elem->type->state_free(elem->state);
    free(elem);
}

void rage_element_process(
        rage_Element const * const elem,
        rage_TransportState const transport_state, uint32_t period_size,
        rage_Ports const * ports) {
    elem->type->process(elem->state, transport_state, period_size, ports);
}
