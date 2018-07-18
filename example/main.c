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
} example_Bits;

static void free_bits(example_Bits * bits) {
    if (bits->cet)
        rage_concrete_element_type_free(bits->cet);
    if (bits->ek)
        rage_element_loader_unload(bits->ek);
    if (bits->el)
        rage_element_loader_free(bits->el);
    if (bits->graph)
        rage_graph_free(bits->graph);
}

static char * input_names[] = {
    "i0",
    "i1"
};

static char * output_names[] = {
    "o0",
    "o1"
};

static rage_BackendPorts ext_ports = {
    .inputs = {.len = 2, .items = input_names},
    .outputs = {.len = 2, .items = output_names}
};

#define RAGE_ABORT_ON_FAILURE(v, r) if (RAGE_FAILED(v)) { \
    printf(#v " failed: %s\n", RAGE_FAILURE_VALUE(v)); \
    free_bits(&bits); \
    return r; \
}

int main() {
    example_Bits bits = {};
    printf("Example started\n");
    rage_NewGraph new_graph = rage_graph_new(ext_ports, 44100);
    RAGE_ABORT_ON_FAILURE(new_graph, 1);
    bits.graph = RAGE_SUCCESS_VALUE(new_graph);
    bits.el = rage_element_loader_new(getenv("RAGE_ELEMENTS_PATH"));
    //rage_ElementTypes element_type_names = rage_element_loader_list(el);
    // FIXME: loading super busted
    rage_ElementKindLoadResult ek_ = rage_element_loader_load(
    //    "./build/libamp.so");
        "./build/libpersist.so");
    RAGE_ABORT_ON_FAILURE(ek_, 2);
    bits.ek = RAGE_SUCCESS_VALUE(ek_);
    printf("Element kind loaded\n");
    // FIXME: Fixed stuff for amp
    rage_Atom tup[] = {
        {.i=1}
    };
    rage_Atom * tupp = tup;
    rage_NewConcreteElementType cet_ = rage_element_type_specialise(
        bits.ek, &tupp);
    RAGE_ABORT_ON_FAILURE(cet_, 3);
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
    printf("Starting graph...\n");
    rage_Error en_st = rage_graph_start_processing(bits.graph);
    RAGE_ABORT_ON_FAILURE(en_st, 4);
    printf("Adding node...\n");
    rage_NewGraphNode new_node = rage_graph_add_node(
        bits.graph, bits.cet, &ts);
    RAGE_ABORT_ON_FAILURE(new_node, 5);
    rage_Error connect_result = rage_graph_connect(
        bits.graph, RAGE_SUCCESS_VALUE(new_node), 0, NULL, 0);
    RAGE_ABORT_ON_FAILURE(connect_result, 6);
    printf("Node added\n");
    sleep(5);
    printf("Recording...\n");
    rage_graph_set_transport_state(bits.graph, RAGE_TRANSPORT_ROLLING);
    printf("Sleeping...\n");
    sleep(5);
    printf("Seeking\n");
    rage_Time t = {.second=2, .fraction=1552};
    rage_graph_set_transport_state(bits.graph, RAGE_TRANSPORT_STOPPED);
    rage_graph_transport_seek(bits.graph, &t);
    rage_graph_set_transport_state(bits.graph, RAGE_TRANSPORT_ROLLING);
    sleep(2);
    printf("Stopping...\n");
    rage_graph_remove_node(bits.graph, RAGE_SUCCESS_VALUE(new_node));
    rage_graph_stop_processing(bits.graph);
    free_bits(&bits);
}
