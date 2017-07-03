#include "graph.h"
#include <stdlib.h>
#include "proc_block.h"

struct rage_Graph {
    uint32_t sample_rate;
    rage_ProcBlock * pb;
};

rage_NewGraph rage_graph_new(uint32_t sample_rate) {
    rage_NewProcBlock npb = rage_proc_block_new(sample_rate);
    if (RAGE_FAILED(npb)) {
        RAGE_FAIL(rage_NewGraph, RAGE_FAILURE_VALUE(npb))
    }
    rage_Graph * g = malloc(sizeof(rage_Graph));
    g->sample_rate = sample_rate;
    g->pb = RAGE_SUCCESS_VALUE(npb);
    RAGE_SUCCEED(rage_NewGraph, g)
}

void rage_graph_free(rage_Graph * g) {
    rage_proc_block_free(g->pb);
    free(g);
}

rage_Error rage_graph_start_processing(rage_Graph * g) {
    return rage_proc_block_start(g->pb);
}

void rage_graph_stop_processing(rage_Graph * g) {
    rage_proc_block_stop(g->pb);
}

void rage_graph_set_transport_state(rage_Graph * g, rage_TransportState s) {
    rage_proc_block_set_transport_state(g->pb, s);
}

struct rage_GraphNode {
    rage_Element * elem;
    rage_Harness * mount;  // FIXME: smells of dodgy naming
};

rage_NewGraphNode rage_graph_add_node(
        rage_Graph * g, rage_ElementType * et, rage_Atom ** type_params,
        // Not sure I like that name seems to be required, is that a good idea?
        char const * name, rage_TimeSeries const * ts) {
    // FIXME: fixed frame size
    rage_ElementNewResult new_elem = rage_element_new(
        et, g->sample_rate, 1024, type_params);
    if (RAGE_FAILED(new_elem)) {
        RAGE_FAIL(rage_NewGraphNode, RAGE_FAILURE_VALUE(new_elem))
    }
    rage_Element * elem = RAGE_SUCCESS_VALUE(new_elem);
    rage_MountResult mr = rage_proc_block_mount(g->pb, elem, ts, name);
    if (RAGE_FAILED(mr)) {
        rage_element_free(elem);
    }
    rage_GraphNode * node = malloc(sizeof(rage_GraphNode));
    node->mount = RAGE_SUCCESS_VALUE(mr);
    node->elem = elem;
    RAGE_SUCCEED(rage_NewGraphNode, node)
}

void rage_graph_remove_node(rage_Graph * g, rage_GraphNode * n) {
    rage_proc_block_unmount(n->mount);
    rage_element_free(n->elem);
    free(n);
}

/* FIXME: connecting story stinks, I think having an alternative "binding"
 * (which should probably be renamed backend) is probably a good idea
rage_NewConnection rage_graph_connect(
        rage_Graph * g, rage_Port * src, rage_Port * dest) {
}
*/
