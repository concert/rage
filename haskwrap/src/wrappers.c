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
    rage_NewGraphNode, (rage_Graph * g, rage_ConcreteElementType * cet, rage_TimeSeries const * ts),
    rage_graph_add_node, (g, cet, ts))

RAGE_HS_STRUCT_WRAPPER(
    rage_Error, (rage_Graph * g, rage_Time const * t),
    rage_graph_transport_seek, (g, t))

#undef RAGE_HS_STRUCT_WRAPPER

rage_InstanceSpec * rage_cet_get_spec_hs(rage_ConcreteElementType * cet) {
    return &cet->spec;
}

rage_NewGraph * rage_graph_new_hs(
        uint32_t sample_rate, rage_BackendDescriptions * inputs,
        rage_BackendDescriptions * outputs) {
    rage_BackendPorts p = {.inputs = *inputs, .outputs = *outputs};
    rage_NewGraph ng = rage_graph_new(p, sample_rate);
    rage_NewGraph * ngp = malloc(sizeof(rage_NewGraph));
    *ngp = ng;
    return ngp;
}
