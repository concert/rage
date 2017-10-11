#include <stdlib.h>
#include <dlfcn.h>
#include <dirent.h>
#include "loader.h"

typedef struct rage_TypeHandle {
    void * dlhandle;
    rage_ElementType const * type;
    struct rage_TypeHandle * next;
} rage_TypeHandle;

struct rage_ElementLoader {
    char * elems_path;
    rage_PluginFilter pf;
    rage_TypeHandle * type_handle;
};

rage_ElementLoader * rage_element_loader_new(char const * elems_path) {
    return rage_fussy_element_loader_new(elems_path, NULL);
}

rage_ElementLoader * rage_fussy_element_loader_new(
        char const * elems_path, rage_PluginFilter pf) {
    rage_ElementLoader * el = malloc(sizeof(rage_ElementLoader));
    el->elems_path = strdup(elems_path);
    el->pf = pf;
    el->type_handle = NULL;
    return el;
}

void rage_element_loader_free(rage_ElementLoader * el) {
    while (el->type_handle != NULL) {
        rage_TypeHandle * nxt = el->type_handle->next;
        dlclose(el->type_handle->dlhandle);
        free(el->type_handle);
        el->type_handle = nxt;
    }
    free(el->elems_path);
    free(el);
}

rage_ElementTypes * rage_element_loader_list(rage_ElementLoader * el) {
    rage_ElementTypes * elems = malloc(sizeof(rage_ElementTypes));
    elems->len = 0;
    elems->items = NULL;
    struct dirent ** entries;
    int n_items = scandir(el->elems_path, &entries, el->pf, NULL);
    if (n_items >= 0) {
        elems->len = n_items;
        elems->items = calloc(n_items, sizeof(char *));
        for (uint32_t i = 0; i < n_items; i++) {
            elems->items[i] = strdup(entries[i]->d_name);
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

static void type_handle_append(
        rage_TypeHandle ** th, void * dlhandle, rage_ElementType const * type) {
    rage_TypeHandle * new_th = malloc(sizeof(rage_TypeHandle));
    new_th->next = NULL;
    new_th->dlhandle = dlhandle;
    new_th->type = type;
    while (*th != NULL) {
        th = &(*th)->next;
    }
    *th = new_th;
}

rage_ElementTypeLoadResult rage_element_loader_load(
        rage_ElementLoader * el, char const * type_name) {
    void * handle = dlopen(type_name, RTLD_LAZY);
    if (handle == NULL)
        return RAGE_FAILURE(rage_ElementTypeLoadResult, dlerror());
    rage_ElementType * type = dlsym(handle, "elem_info");
    #define RAGE_ETL_BAIL(msg) \
        dlclose(handle); \
        return RAGE_FAILURE(rage_ElementTypeLoadResult, msg);
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
    RAGE_ETL_MANDATORY_PARAM(get_ports)
    RAGE_ETL_MANDATORY_PARAM(free_ports)
    RAGE_ETL_MANDATORY_PARAM(process)
    #undef RAGE_ETL_MANDATORY_PARAM
    if (type->prep != NULL && type->clear == NULL) {
        RAGE_ETL_BAIL("Prep provide but no clear")
    }
    #undef RAGE_ETL_BAIL
    type_handle_append(&el->type_handle, handle, type);
    return RAGE_SUCCESS(rage_ElementTypeLoadResult, type);
}

void rage_element_loader_unload(
        rage_ElementLoader * el, rage_ElementType * type) {
    rage_TypeHandle ** prev = &el->type_handle;
    rage_TypeHandle * th = el->type_handle;
    while (th != NULL) {
        if (th->type == type) {
            dlclose(th->dlhandle);
            *prev = th->next;
            free(th);
            th = NULL;  // Better or worse than break?
        } else {
            prev = &th->next;
            th = th->next;
        }
    }
}

// ConcreteElementType

rage_NewConcreteElementType rage_element_type_specialise(
        rage_ElementType * type, rage_Atom ** params) {
    rage_NewInstanceSpec new_ports = type->get_ports(params);
    if (RAGE_FAILED(new_ports))
        return RAGE_FAILURE_CAST(rage_NewConcreteElementType, new_ports);
    rage_InstanceSpec spec = RAGE_SUCCESS_VALUE(new_ports);
    rage_ConcreteElementType * cet = malloc(sizeof(rage_ConcreteElementType));
    cet->type = type;
    cet->params = params;
    cet->spec = spec;
    return RAGE_SUCCESS(rage_NewConcreteElementType, cet);
}

void rage_concrete_element_type_free(rage_ConcreteElementType * cet) {
    cet->type->free_ports(cet->spec);
    free(cet);
}

// Element

rage_ElementNewResult rage_element_new(
        rage_ConcreteElementType * cet, uint32_t sample_rate,
        uint32_t frame_size) {
    rage_NewElementState new_state = cet->type->state_new(
        sample_rate, frame_size, cet->params);
    if (RAGE_FAILED(new_state))
        return RAGE_FAILURE_CAST(rage_ElementNewResult, new_state);
    void * state = RAGE_SUCCESS_VALUE(new_state);
    rage_Element * const elem = malloc(sizeof(rage_Element));
    elem->cet = cet;
    elem->state = state;
    return RAGE_SUCCESS(rage_ElementNewResult, elem);
}

void rage_element_free(rage_Element * elem) {
    elem->cet->type->state_free(elem->state);
    free(elem);
}

void rage_element_process(
        rage_Element const * const elem,
        rage_TransportState const transport_state, rage_Ports const * ports) {
    elem->cet->type->process(elem->state, transport_state, ports);
}
