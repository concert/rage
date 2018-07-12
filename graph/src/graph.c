#include "graph.h"
#include <stdlib.h>
#include "proc_block.h"
#include "jack_bindings.h"

struct rage_Graph {
    uint32_t sample_rate;
    rage_ProcBlock * pb;
    rage_JackBackend * jb;
};

rage_NewGraph rage_graph_new(uint32_t sample_rate) {
    rage_ProcBlock * pb = rage_proc_block_new(sample_rate, RAGE_TRANSPORT_STOPPED);
    rage_NewJackBackend njb = rage_jack_backend_new(
        rage_proc_block_get_backend_config(pb));
    if (RAGE_FAILED(njb)) {
        rage_proc_block_free(pb);
        return RAGE_FAILURE_CAST(rage_NewGraph, njb);
    }
    rage_Graph * g = malloc(sizeof(rage_Graph));
    g->sample_rate = sample_rate;
    g->pb = pb;
    g->jb = RAGE_SUCCESS_VALUE(njb);
    return RAGE_SUCCESS(rage_NewGraph, g);
}

void rage_graph_free(rage_Graph * g) {
    rage_proc_block_free(g->pb);
    rage_jack_backend_free(g->jb);
    free(g);
}

rage_Error rage_graph_start_processing(rage_Graph * g) {
    rage_Error err = rage_proc_block_start(g->pb);
    if (!RAGE_FAILED(err)) {
        err = rage_jack_backend_activate(g->jb);
    }
    return err;
}

void rage_graph_stop_processing(rage_Graph * g) {
    rage_jack_backend_deactivate(g->jb);
    rage_proc_block_stop(g->pb);
}

void rage_graph_set_transport_state(rage_Graph * g, rage_TransportState s) {
    rage_proc_block_set_transport_state(g->pb, s);
}

rage_Error rage_graph_transport_seek(rage_Graph * g, rage_Time const * target) {
    return rage_proc_block_transport_seek(g->pb, rage_time_to_frame(target, g->sample_rate));
}

struct rage_GraphNode {
    rage_Element * elem;
    rage_Harness * harness;
};

rage_NewGraphNode rage_graph_add_node(
        rage_Graph * g, rage_ConcreteElementType * cet,
        rage_TimeSeries const * ts) {
    rage_ElementNewResult new_elem = rage_element_new(cet, g->sample_rate);
    if (RAGE_FAILED(new_elem)) {
        return RAGE_FAILURE_CAST(rage_NewGraphNode, new_elem);
    }
    rage_Element * elem = RAGE_SUCCESS_VALUE(new_elem);
    rage_MountResult mr = rage_proc_block_mount(g->pb, elem, ts);
    if (RAGE_FAILED(mr)) {
        rage_element_free(elem);
    }
    rage_GraphNode * node = malloc(sizeof(rage_GraphNode));
    node->harness = RAGE_SUCCESS_VALUE(mr);
    node->elem = elem;
    return RAGE_SUCCESS(rage_NewGraphNode, node);
}

void rage_graph_remove_node(rage_Graph * g, rage_GraphNode * n) {
    rage_proc_block_unmount(n->harness);
    rage_element_free(n->elem);
    free(n);
}

static rage_Harness * rage_graph_get_harness(rage_GraphNode * n) {
    return (n == NULL) ? NULL : n->harness;
}

rage_Error rage_graph_connect(
        rage_Graph * g,
        rage_GraphNode * source, uint32_t source_idx,
        rage_GraphNode * sink, uint32_t sink_idx) {
    return rage_proc_block_connect(
        g->pb,
        rage_graph_get_harness(source), source_idx,
        rage_graph_get_harness(sink), sink_idx);
}
