#include <stdbool.h>

#include "wrappers.h"
#include "refcounter.h"
#include "error.h"
#include "graph.h"
#include "loader.h"

typedef RAGE_HS_COUNTABLE(rage_ElementKind, rage_hs_ElementKind) rage_hs_ElementKindRc;

rage_hs_ElementKindLoadResult * rage_hs_element_loader_load(char * name) {
    rage_hs_ElementKindLoadResult * r = malloc(sizeof(rage_hs_ElementKindLoadResult));
    rage_ElementKindLoadResult eklr = rage_element_loader_load(name);
    if (RAGE_FAILED(eklr)) {
        *r = RAGE_FAILURE_CAST(rage_hs_ElementKindLoadResult, eklr);
    } else {
        RAGE_HS_COUNT(krc, rage_hs_ElementKindRc, rage_element_loader_unload, RAGE_SUCCESS_VALUE(eklr))
        *r = RAGE_SUCCESS(rage_hs_ElementKindLoadResult, krc.external);
    }
    return r;
}

void rage_hs_element_loader_unload(rage_hs_ElementKind * k) {
    rage_hs_ElementKindRc r = {.external = k};
    RAGE_HS_DECREMENT_REF(r);
}

rage_ParamDefList const * rage_hs_element_kind_parameters(rage_hs_ElementKind * k) {
    rage_hs_ElementKindRc r = {.external = k};
    return rage_element_kind_parameters(RAGE_HS_REF(r));
}

typedef RAGE_HS_COUNTABLE(rage_ConcreteElementType, rage_hs_ConcreteElementType) rage_hs_ConcreteElementTypeRc;

rage_hs_ConcreteElementTypeResult * rage_hs_element_type_specialise(
        rage_hs_ElementKind * k, rage_Atom ** params) {
    rage_hs_ConcreteElementTypeResult * r = malloc(sizeof(rage_hs_ConcreteElementTypeResult));
    rage_hs_ElementKindRc krc = {.external = k};
    rage_NewConcreteElementType cetr = rage_element_type_specialise(RAGE_HS_REF(krc), params);
    if (RAGE_FAILED(cetr)) {
        *r = RAGE_FAILURE_CAST(rage_hs_ConcreteElementTypeResult, cetr);
    } else {
        RAGE_HS_COUNT(
            cetrc, rage_hs_ConcreteElementTypeRc,
            rage_concrete_element_type_free, RAGE_SUCCESS_VALUE(cetr))
        RAGE_HS_DEPEND_REF(cetrc, krc);
        *r = RAGE_SUCCESS(rage_hs_ConcreteElementTypeResult, cetrc.external);
    }
    return r;
}

void rage_hs_concrete_element_type_free(rage_hs_ConcreteElementType * cet) {
    rage_hs_ConcreteElementTypeRc r = {.external = cet};
    RAGE_HS_DECREMENT_REF(r);
}

rage_InstanceSpec * rage_hs_cet_get_spec(rage_hs_ConcreteElementType * cet) {
    rage_hs_ConcreteElementTypeRc r = {.external = cet};
    return &RAGE_HS_REF(r)->spec;
}

typedef RAGE_HS_COUNTABLE(rage_Graph, rage_hs_Graph) rage_hs_GraphRc;

rage_hs_NewGraph * rage_hs_graph_new(
        uint32_t sample_rate, rage_BackendDescriptions * inputs,
        rage_BackendDescriptions * outputs) {
    rage_BackendPorts p = {.inputs = *inputs, .outputs = *outputs};
    rage_NewGraph const ng = rage_graph_new(p, sample_rate);
    rage_hs_NewGraph * rv = malloc(sizeof(rage_hs_NewGraph));
    if (RAGE_FAILED(ng)) {
        *rv = RAGE_FAILURE_CAST(rage_hs_NewGraph, ng);
    } else {
        RAGE_HS_COUNT(grc, rage_hs_GraphRc, rage_graph_free, RAGE_SUCCESS_VALUE(ng));
        *rv = RAGE_SUCCESS(rage_hs_NewGraph, grc.external);
    }
    return rv;
}

typedef struct {
    rage_Graph * g;
    rage_GraphNode * n;
    bool explicitly_removed;
} rage_hs_GraphNodeWrapper;

static void rage_hs_internal_graph_remove_node(rage_hs_GraphNodeWrapper * w) {
    rage_graph_remove_node(w->g, w->n);
    free(w);
}

typedef RAGE_HS_COUNTABLE(rage_hs_GraphNodeWrapper, rage_hs_GraphNode) rage_hs_GraphNodeRc;

rage_hs_NewGraphNode * rage_hs_graph_add_node(
        rage_hs_Graph * hsg, rage_hs_ConcreteElementType * cetp,
        rage_TimeSeries const * ts) {
    rage_hs_GraphRc grc = {.external = hsg};
    rage_hs_ConcreteElementTypeRc cetrc = {.external = cetp};
    rage_NewGraphNode const ngn = rage_graph_add_node(RAGE_HS_REF(grc), RAGE_HS_REF(cetrc), ts);
    rage_hs_NewGraphNode * rv = malloc(sizeof(rage_hs_NewGraphNode));
    if (RAGE_FAILED(ngn)) {
        *rv = RAGE_FAILURE_CAST(rage_hs_NewGraphNode, ngn);
    } else {
        rage_GraphNode * const gn = RAGE_SUCCESS_VALUE(ngn);
        rage_hs_GraphNodeWrapper * w = malloc(sizeof(rage_hs_GraphNodeWrapper));
        w->g = RAGE_HS_REF(grc);
        w->n = gn;
        w->explicitly_removed = false;
        RAGE_HS_COUNT(gnrc, rage_hs_GraphNodeRc, rage_hs_internal_graph_remove_node, w)
        RAGE_HS_DEPEND_REF(gnrc, cetrc);
        RAGE_HS_DEPEND_REF(gnrc, grc);
        *rv = RAGE_SUCCESS(rage_hs_NewGraphNode, gnrc.external);
    }
    return rv;
}

void rage_hs_graph_node_free(rage_hs_GraphNode * gn) {
    rage_hs_GraphNodeRc r = {.external = gn};
    rage_hs_GraphNodeWrapper * w = RAGE_HS_REF(r);
    if (!w->explicitly_removed) {
        RAGE_HS_DECREMENT_REF(r);
    }
    free(w);
}

void rage_hs_graph_remove_node(rage_hs_GraphNode * gn) {
    rage_hs_GraphNodeRc r = {.external = gn};
    rage_hs_GraphNodeWrapper * w = RAGE_HS_REF(r);
    if (!w->explicitly_removed) {
        w->explicitly_removed = true;
        RAGE_HS_DECREMENT_REF(r);
    }
}
