#include <stdio.h>
#include <unistd.h> // for sleep
#include "error.h"
#include "graph.h"
#include "loader.h"

int main() {
    printf("Example started\n");
    rage_NewGraph new_graph = rage_graph_new(44100);
    if (RAGE_FAILED(new_graph)) {
        printf("Graph init failed: %s\n", RAGE_FAILURE_VALUE(new_graph));
        return 1;
    }
    rage_Graph * graph = RAGE_SUCCESS_VALUE(new_graph);
    rage_ElementLoader * el = rage_element_loader_new();
    //rage_ElementTypes element_type_names = rage_element_loader_list(el);
    // FIXME: loading super busted
    rage_ElementTypeLoadResult et_ = rage_element_loader_load(
    //    el, "./build/libamp.so");
        el, "./build/libpersist.so");
    if (RAGE_FAILED(et_)) {
        printf("Element type load failed: %s\n", RAGE_FAILURE_VALUE(et_));
        rage_graph_free(graph);
        rage_element_loader_free(el);
        return 2;
    }
    rage_ElementType * const et = RAGE_SUCCESS_VALUE(et_);
    printf("Element type loaded\n");
    // FIXME: Fixed stuff for amp
    rage_Atom tup[] = {
        {.i=1}
    };
    rage_Atom * tupp = tup;
    rage_NewConcreteElementType cet_ = rage_element_type_specialise(
        et, &tupp);
    if (RAGE_FAILED(cet_)) {
        printf("Element type specialising failed: %s\n", RAGE_FAILURE_VALUE(cet_));
        rage_graph_free(graph);
        rage_element_loader_unload(el, et);
        rage_element_loader_free(el);
	return 3;
    }
    rage_ConcreteElementType * cet = RAGE_SUCCESS_VALUE(
        rage_element_type_specialise(et, &tupp));
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
        graph, cet, "persistance", &ts);
    if (RAGE_FAILED(new_node)) {
        printf("Node creation failed: %s\n", RAGE_FAILURE_VALUE(new_node));
        rage_graph_free(graph);
        rage_element_loader_free(el);
        return 4;
    }
    printf("Element loaded\n");
    //FIXME: handle errors (start/stop)
    printf("Starting graph...\n");
    rage_Error en_st = rage_graph_start_processing(graph);
    if (RAGE_FAILED(en_st)) {
        printf("Start failed: %s\n", RAGE_FAILURE_VALUE(en_st));
    } else {
        printf("Recording...\n");
        rage_graph_set_transport_state(graph, RAGE_TRANSPORT_ROLLING);
        printf("Sleeping...\n");
        sleep(9);
        printf("Stopping...\n");
        rage_graph_stop_processing(graph);
    }
    rage_graph_remove_node(graph, RAGE_SUCCESS_VALUE(new_node));
    printf("Elem freed\n");
    rage_concrete_element_type_free(cet);
    rage_element_loader_unload(el, et);
    printf("Elem type freed\n");
    rage_graph_free(graph);
    rage_element_loader_free(el);
}
