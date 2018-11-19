#include "error.h"
#include "graph.h"
#include "graph_test_factories.h"

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

rage_Atom half_vol[] = {{.f = 0.5}};
rage_TimePoint example_points[] = {
    {
        .time = {},
        .value = half_vol,
        .mode = RAGE_INTERPOLATION_CONST
    }
};
rage_TimeSeries example_series = {.len = 1, .items = example_points};

static rage_Error test_graph() {
    rage_NewGraph new_graph = rage_graph_new(example_ports, 44100);
    if (RAGE_FAILED(new_graph))
        return RAGE_FAILURE_CAST(rage_Error, new_graph);
    rage_Graph * graph = RAGE_SUCCESS_VALUE(new_graph);
    rage_Error rv = rage_graph_start_processing(graph);
    if (!RAGE_FAILED(rv)) {
        rage_NewTestElem nte = new_test_elem("./libamp.so");
        if (RAGE_FAILED(nte)) {
            rv = RAGE_FAILURE_CAST(rage_Error, nte);
        } else {
            rage_TestElem te = RAGE_SUCCESS_VALUE(nte);
            rage_NewGraphNode ngn = rage_graph_add_node(graph, te.cet, &example_series);
            if (RAGE_FAILED(ngn)) {
                rv = RAGE_FAILURE_CAST(rage_Error, ngn);
            } else {
                rage_GraphNode * n = RAGE_SUCCESS_VALUE(ngn);
                rage_Time target = {.second = 12};
                rv = rage_graph_transport_seek(graph, &target);
                rage_graph_set_transport_state(graph, RAGE_TRANSPORT_ROLLING);
                rage_graph_remove_node(graph, n);
            }
            free_test_elem(te);
        }
        rage_graph_stop_processing(graph);
    }
    rage_graph_free(graph);
    return rv;
}
