#include <stdio.h>
#include <unistd.h> // for sleep
#include "error.h"
#include "graph.h"
#include "loader.h"

typedef struct {
    rage_Graph * graph;
    rage_ElementLoader * el;
    rage_ElementKind * ek;
    rage_ConcreteElementType * cet;
    rage_GraphNode * node;
} example_Bits;

static void free_bits(example_Bits * bits) {
    if (bits->node)
        rage_graph_remove_node(bits->graph, bits->node);
    if (bits->cet)
        rage_concrete_element_type_free(bits->cet);
    if (bits->ek)
        rage_element_loader_unload(bits->ek);
    if (bits->el)
        rage_element_loader_free(bits->el);
    if (bits->graph)
        rage_graph_free(bits->graph);
}

int main() {
    example_Bits bits = {};
    printf("Example started\n");
    rage_NewGraph new_graph = rage_graph_new(44100);
    if (RAGE_FAILED(new_graph)) {
        printf("Graph init failed: %s\n", RAGE_FAILURE_VALUE(new_graph));
        return 1;
    }
    bits.graph = RAGE_SUCCESS_VALUE(new_graph);
    bits.el = rage_element_loader_new(getenv("RAGE_ELEMENTS_PATH"));
    //rage_ElementTypes element_type_names = rage_element_loader_list(el);
    // FIXME: loading super busted
    rage_ElementKindLoadResult ek_ = rage_element_loader_load(
    //    "./build/libamp.so");
        "./build/libpersist.so");
    if (RAGE_FAILED(ek_)) {
        printf("Element kind load failed: %s\n", RAGE_FAILURE_VALUE(ek_));
        free_bits(&bits);
        return 2;
    }
    bits.ek = RAGE_SUCCESS_VALUE(ek_);
    printf("Element kind loaded\n");
    // FIXME: Fixed stuff for amp
    rage_Atom tup[] = {
        {.i=1}
    };
    rage_Atom * tupp = tup;
    rage_NewConcreteElementType cet_ = rage_element_type_specialise(
        bits.ek, &tupp);
    if (RAGE_FAILED(cet_)) {
        printf("Element type specialising failed: %s\n", RAGE_FAILURE_VALUE(cet_));
        free_bits(&bits);
        return 3;
    }
    bits.cet = RAGE_SUCCESS_VALUE(cet_);
    //rage_Atom vals[] = {{.f = 1.0}};
    rage_Atom vals[] = {
        {.e = 0},
        {.s = ""},
        {.t = (rage_Time) {}}
    };
    rage_Atom recbit[] = {
        {.e = 2},
        {.s = "rechere.wav"},
        {.t = (rage_Time) {}}
    };
    rage_Atom terminus[] = {
        {.e = 0},
        {.s = ""},
        {.t = (rage_Time) {}}
    };
    rage_TimePoint tps[] = {
        {
            .time = {.second = 0},
            .value = &(vals[0]),
            .mode = RAGE_INTERPOLATION_CONST
        },
        {
            .time = {.second = 1},
            .value = &(recbit[0]),
            .mode = RAGE_INTERPOLATION_CONST
        },
        {
            .time = {.second = 7},
            .value = &(terminus[0]),
            .mode = RAGE_INTERPOLATION_CONST
        }
    };
    rage_TimeSeries ts = {
        .len = 3,
        .items = tps
    };
    rage_NewGraphNode new_node = rage_graph_add_node(
        bits.graph, bits.cet, "persistance", &ts);
    if (RAGE_FAILED(new_node)) {
        printf("Node creation failed: %s\n", RAGE_FAILURE_VALUE(new_node));
        free_bits(&bits);
        return 4;
    }
    bits.node = RAGE_SUCCESS_VALUE(new_node);
    printf("Element loaded\n");
    //FIXME: handle errors (start/stop)
    printf("Starting graph...\n");
    rage_Error en_st = rage_graph_start_processing(bits.graph);
    if (RAGE_FAILED(en_st)) {
        printf("Start failed: %s\n", RAGE_FAILURE_VALUE(en_st));
    } else {
        printf("Recording...\n");
        rage_graph_set_transport_state(bits.graph, RAGE_TRANSPORT_ROLLING);
        printf("Sleeping...\n");
        sleep(9);
        printf("Stopping...\n");
        rage_graph_stop_processing(bits.graph);
    }
    free_bits(&bits);
}
