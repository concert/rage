#include <stdlib.h>
#include <dlfcn.h>
#include "loader.h"

rage_ElementLoader rage_element_loader_new() {
    return NULL;
}

void rage_element_loader_free(rage_ElementLoader el) {
}

// FIXME: Dynamism
static char amp[] = "./libamp.so";
static char * element_types[] = {amp};

rage_ElementTypes rage_element_loader_list(rage_ElementLoader el) {
    return (rage_ElementTypes) {.len=1, .items=element_types};
}

rage_ElementTypeLoadResult rage_element_loader_load(
        rage_ElementLoader el, char const * type_name) {
    void * handle = dlopen(type_name, RTLD_LAZY);
    if (handle == NULL)
        RAGE_FAIL(rage_ElementTypeLoadResult, dlerror());
    rage_ElementType * type = malloc(sizeof(rage_ElementType));
    type->dlhandle = handle;
    #define RAGE_ETL_MANDATORY_PARAM(struct_name, sym_name) \
        type->struct_name = dlsym(handle, #sym_name); \
        if (type->struct_name == NULL) \
            RAGE_FAIL(rage_ElementTypeLoadResult, dlerror())
    RAGE_ETL_MANDATORY_PARAM(parameters, init_params)
    RAGE_ETL_MANDATORY_PARAM(state_new, elem_new)
    RAGE_ETL_MANDATORY_PARAM(state_free, elem_free)
    RAGE_ETL_MANDATORY_PARAM(run, elem_process)
    #undef RAGE_ETL_MANDATORY_PARAM
    RAGE_SUCCEED(rage_ElementTypeLoadResult, type)
}

void rage_element_loader_unload(
        rage_ElementLoader el, rage_ElementType * type) {
    dlclose(type->dlhandle);
}

rage_ElementNewResult rage_element_new(
        rage_ElementType * type, rage_Tuple params) {
    rage_NewElementState new_state = type->state_new(params);
    RAGE_EXTRACT_VALUE(rage_ElementNewResult, new_state, void *, state)
    rage_PortDescription * pd = type->get_ports(state);
    rage_Element * const elem = malloc(sizeof(rage_Element));
    elem->type = type;
    elem->state = state;
    elem->ports = pd;
    RAGE_SUCCEED(rage_ElementNewResult, elem);
}

void rage_element_free(rage_Element * elem) {
    rage_port_description_free(elem->ports);
    elem->type->state_free(elem->state);
    free(elem);
}
