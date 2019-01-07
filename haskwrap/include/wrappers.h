#pragma once
#define RAGE_DISABLE_ANON_UNIONS 1
#include "loader.h"
#include "graph.h"

typedef struct rage_hs_ElementKind rage_hs_ElementKind;
typedef RAGE_OR_ERROR(rage_hs_ElementKind *) rage_hs_ElementKindLoadResult;

rage_hs_ElementKindLoadResult * rage_hs_element_loader_load(char * name);
void rage_hs_element_loader_unload(rage_hs_ElementKind * k);

typedef struct rage_hs_ConcreteElementType rage_hs_ConcreteElementType;
typedef RAGE_OR_ERROR(rage_hs_ConcreteElementType *) rage_hs_ConcreteElementTypeResult;

rage_hs_ConcreteElementTypeResult * rage_hs_element_type_specialise(rage_hs_ElementKind *k, rage_Atom ** params);
void rage_hs_concrete_element_type_free(rage_hs_ConcreteElementType * cet);

rage_InstanceSpec * rage_hs_cet_get_spec(rage_hs_ConcreteElementType * cetr);

rage_ParamDefList const * rage_hs_element_kind_parameters(rage_hs_ElementKind *k);

typedef struct rage_hs_GraphNode rage_hs_GraphNode;
typedef RAGE_OR_ERROR(rage_hs_GraphNode *) rage_hs_NewGraphNode;

rage_hs_NewGraphNode * rage_hs_graph_add_node(
    rage_Graph * g, rage_hs_ConcreteElementType * cetp,
    rage_TimeSeries const * ts);

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
rage_Error * rage_graph_connect_hs(
    rage_ConTrans * g,
    rage_GraphNode * source, uint32_t source_idx,
    rage_GraphNode * sink, uint32_t sink_idx);
rage_Error * rage_graph_disconnect_hs(
    rage_ConTrans * g,
    rage_GraphNode * source, uint32_t source_idx,
    rage_GraphNode * sink, uint32_t sink_idx);

// Wrappers for things that use anonymous unions
rage_InstanceSpec * rage_cet_get_spec_hs(rage_ConcreteElementType * cet);
