#pragma once
#define RAGE_DISABLE_ANON_UNIONS 1
#include "loader.h"
#include "graph.h"

// Wrappers for haskell runtime only accepting pointers
rage_ElementKindLoadResult * rage_element_loader_load_hs(char * name);
rage_NewConcreteElementType * rage_element_type_specialise_hs(rage_ElementKind * t, rage_Atom ** params);
rage_NewGraph * rage_graph_new_hs(
    uint32_t sample_rate, rage_BackendDescriptions * inputs,
    rage_BackendDescriptions * outputs);
rage_NewGraphNode * rage_graph_add_node_hs(
    rage_Graph * g, rage_ConcreteElementType * cet,
    rage_TimeSeries const * ts);
rage_Error * rage_graph_transport_seek_hs(rage_Graph * g, rage_Time const * t);

// Wrappers for things that use anonymous unions
rage_InstanceSpec * rage_cet_get_spec_hs(rage_ConcreteElementType * cet);
