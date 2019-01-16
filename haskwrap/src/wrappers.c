#include "wrappers.h"
#include "refcounter.h"
#include "error.h"
#include "graph.h"
#include "loader.h"

rage_hs_ElementKindLoadResult * rage_hs_element_loader_load(char * name) {
    rage_hs_ElementKindLoadResult * r = malloc(sizeof(rage_hs_ElementKindLoadResult));
    rage_ElementKindLoadResult eklr = rage_element_loader_load(name);
    if (RAGE_FAILED(eklr)) {
        *r = RAGE_FAILURE_CAST(rage_hs_ElementKindLoadResult, eklr);
    } else {
        rage_hs_RefCount * rc = RAGE_HS_COUNT_REF(rage_element_loader_unload, RAGE_SUCCESS_VALUE(eklr));
        *r = RAGE_SUCCESS(rage_hs_ElementKindLoadResult, (rage_hs_ElementKind *) rc);
    }
    return r;
}

void rage_hs_element_loader_unload(rage_hs_ElementKind * k) {
    rage_hs_decrement_ref((rage_hs_RefCount *) k);
}

rage_ParamDefList const * rage_hs_element_kind_parameters(rage_hs_ElementKind * k) {
    return rage_element_kind_parameters(rage_hs_ref((rage_hs_RefCount *) k));
}

rage_hs_ConcreteElementTypeResult * rage_hs_element_type_specialise(rage_hs_ElementKind * k, rage_Atom ** params) {
    rage_hs_ConcreteElementTypeResult * r = malloc(sizeof(rage_hs_ConcreteElementTypeResult));
    rage_hs_RefCount * krc = (rage_hs_RefCount *) k;
    rage_NewConcreteElementType cetr = rage_element_type_specialise(rage_hs_ref(krc), params);
    if (RAGE_FAILED(cetr)) {
        *r = RAGE_FAILURE_CAST(rage_hs_ConcreteElementTypeResult, cetr);
    } else {
        rage_hs_RefCount * cetrc = RAGE_HS_COUNT_REF(
            rage_concrete_element_type_free, RAGE_SUCCESS_VALUE(cetr));
        rage_hs_depend_ref(cetrc, krc);
        *r = RAGE_SUCCESS(
            rage_hs_ConcreteElementTypeResult,
            (rage_hs_ConcreteElementType *) cetrc);
    }
    return r;
}

void rage_hs_concrete_element_type_free(rage_hs_ConcreteElementType * cet) {
    rage_hs_decrement_ref((rage_hs_RefCount *) cet);
}

rage_InstanceSpec * rage_hs_cet_get_spec(rage_hs_ConcreteElementType * cetr) {
    rage_ConcreteElementType * cet = rage_hs_ref((rage_hs_RefCount *) cetr);
    return &cet->spec;
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
} rage_hs_GraphNodeWrapper;

static void rage_hs_internal_graph_remove_node(void * vp) {
    rage_hs_GraphNodeWrapper * w = vp;
    rage_graph_remove_node(w->g, w->n);
    free(w);
}

typedef RAGE_HS_COUNTABLE(rage_GraphNode, rage_hs_GraphNode) rage_hs_GraphNodeRc;

rage_hs_NewGraphNode * rage_hs_graph_add_node(
        rage_hs_Graph * hsg, rage_hs_ConcreteElementType * cetp,
        rage_TimeSeries const * ts) {
    rage_hs_GraphRc grc = {.external = hsg};
    rage_hs_RefCount * cetr = (rage_hs_RefCount *) cetp;
    rage_ConcreteElementType * cet = rage_hs_ref(cetr);
    rage_NewGraphNode const ngn = rage_graph_add_node(RAGE_HS_REF(grc), cet, ts);
    rage_hs_NewGraphNode * rv = malloc(sizeof(rage_hs_NewGraphNode));
    if (RAGE_FAILED(ngn)) {
        *rv = RAGE_FAILURE_CAST(rage_hs_NewGraphNode, ngn);
    } else {
        rage_GraphNode * const gn = RAGE_SUCCESS_VALUE(ngn);
        rage_hs_GraphNodeWrapper * w = malloc(sizeof(rage_hs_GraphNodeWrapper));
        w->g = RAGE_HS_REF(grc);
        w->n = gn;
        rage_hs_RefCount * gnrc = rage_hs_count_ref(
            rage_hs_internal_graph_remove_node, w);
        rage_hs_depend_ref(gnrc, cetr);
        rage_hs_depend_ref(gnrc, grc.rc);
        *rv = RAGE_SUCCESS(rage_hs_NewGraphNode, (rage_hs_GraphNode *) gnrc);
    }
    return rv;
}

void rage_hs_graph_remove_node(rage_hs_GraphNode * hgn) {
    rage_hs_GraphNodeRc r = {.external = hgn};
    RAGE_HS_DECREMENT_REF(r);
}
