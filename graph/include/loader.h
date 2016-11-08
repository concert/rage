#pragma once

#include "atoms.h"
#include "element.h"
#include "error.h"
#include "macros.h"

typedef struct {
    void * dlhandle;
    rage_TupleDef const * parameters;
    rage_ElementGetPortsDescription get_ports;
    rage_ElementStateNew state_new;
    rage_ElementProcess run;
    rage_ElementStateFree state_free;
} rage_ElementType;

typedef struct {
    rage_ElementType * type;
    void * state;
    rage_PortDescription * ports;
} rage_Element;

typedef void * rage_ElementLoader;

rage_ElementLoader rage_element_loader_new();
void rage_element_loader_free(rage_ElementLoader el);

typedef RAGE_ARRAY(char *) rage_ElementTypes;
rage_ElementTypes rage_element_loader_list(rage_ElementLoader el);
typedef RAGE_OR_ERROR(rage_ElementType *) rage_ElementTypeLoadResult;
rage_ElementTypeLoadResult rage_element_loader_load(
    rage_ElementLoader el, char const * type_name);
void rage_element_loader_unload(rage_ElementLoader el, rage_ElementType * type);

typedef RAGE_OR_ERROR(rage_Element *) rage_ElementNewResult;
rage_ElementNewResult rage_element_new(rage_ElementType * type, rage_Tuple params);
void rage_element_free(rage_Element * elem);
rage_Error rage_element_prepare(rage_Element * elem, rage_Tuple params);
void rage_element_halt(rage_Element * elem);
void rage_element_free(rage_Element * elem);

// rage_PortsDescription rage_element_get_ports(rage_Element const * const elem);
