#include "error.h"
#include "graph.h"

static char * example_input_names[] = {
    "in0"
};

static char * example_output_names[] = {
    "out0"
};

static rage_BackendPorts example_ports = {
    .inputs = {.len = 1, .items = example_input_names},
    .outputs = {.len = 1, .items = example_output_names}
};

static rage_Error test_graph() {
    rage_NewGraph new_graph = rage_graph_new(example_ports, 44100);
    if (RAGE_FAILED(new_graph))
        return RAGE_FAILURE_CAST(rage_Error, new_graph);
    rage_Graph * graph = RAGE_SUCCESS_VALUE(new_graph);
    rage_Error rv = rage_graph_start_processing(graph);
    if (!RAGE_FAILED(rv)) {
        rage_Time target = {.second = 12};
        rv = rage_graph_transport_seek(graph, &target);
        rage_graph_set_transport_state(graph, RAGE_TRANSPORT_ROLLING);
        rage_graph_stop_processing(graph);
    }
    rage_graph_free(graph);
    return rv;
}
