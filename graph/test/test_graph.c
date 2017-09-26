#include "error.h"
#include "graph.h"
#include "test_factories.h"
#include "pdls.c"

static rage_Error test_graph() {
    rage_NewGraph new_graph = rage_graph_new(44100);
    if (RAGE_FAILED(new_graph))
        return RAGE_FAILURE_CAST(rage_Error, new_graph);
    rage_Graph * graph = RAGE_SUCCESS_VALUE(new_graph);
    rage_Error rv = rage_graph_start_processing(graph);
    if (!RAGE_FAILED(rv)) {
        rage_graph_set_transport_state(graph, RAGE_TRANSPORT_ROLLING);
        rage_graph_stop_processing(graph);
    }
    rage_graph_free(graph);
    return rv;
}
