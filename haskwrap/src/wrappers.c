#include "wrappers.h"

#define RAGE_HS_STRUCT_WRAPPER(t, params, wrapped, wargs) \
    t * wrapped ## _hs params { \
        t * p = malloc(sizeof(t)); \
        *p = wrapped wargs; \
        return p; \
    }

RAGE_HS_STRUCT_WRAPPER(
    rage_ElementKindLoadResult, (char * name),
    rage_element_loader_load, (name))

RAGE_HS_STRUCT_WRAPPER(
    rage_NewConcreteElementType, (rage_ElementKind * k, rage_Atom ** params),
    rage_element_type_specialise, (k, params))

RAGE_HS_STRUCT_WRAPPER(
    rage_NewGraph, (uint32_t sample_rate),
    rage_graph_new, (sample_rate))

RAGE_HS_STRUCT_WRAPPER(
    rage_NewGraphNode, (rage_Graph * g, rage_ConcreteElementType * cet, char const * name, rage_TimeSeries const * ts),
    rage_graph_add_node, (g, cet, name, ts))

#undef RAGE_HS_STRUCT_WRAPPER
