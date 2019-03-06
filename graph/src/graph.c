#include "graph.h"
#include <stdlib.h>
#include <pthread.h>
#include "proc_block.h"
#include "jack_bindings.h"
#include "queue.h"
#include "event.h"

struct rage_Graph {
    uint32_t sample_rate;
    rage_Queue * evt_q;
    pthread_t q_thread;
    rage_ProcBlock * pb;
    rage_JackBackend * jb;
};

rage_NewGraph rage_graph_new(rage_BackendPorts ports, uint32_t sample_rate) {
    rage_Queue * evt_q = rage_queue_new();
    rage_ProcBlock * pb = rage_proc_block_new(
        sample_rate, 1024, ports, RAGE_TRANSPORT_STOPPED, evt_q);
    rage_NewJackBackend njb = rage_jack_backend_new(
        rage_proc_block_get_backend_config(pb));
    if (RAGE_FAILED(njb)) {
        rage_proc_block_free(pb);
        rage_queue_free(evt_q);
        return RAGE_FAILURE_CAST(rage_NewGraph, njb);
    }
    rage_Graph * g = malloc(sizeof(rage_Graph));
    g->sample_rate = sample_rate;
    g->pb = pb;
    g->jb = RAGE_SUCCESS_VALUE(njb);
    g->evt_q = evt_q;
    return RAGE_SUCCESS(rage_NewGraph, g);
}

void rage_graph_free(rage_Graph * g) {
    rage_proc_block_free(g->pb);
    rage_jack_backend_free(g->jb);
    rage_queue_free(g->evt_q);
    free(g);
}

typedef struct {
    rage_Queue * q;
    rage_EventCb cb;
    void * ctx;
} rage_EventCbInfo;

rage_EventType * rage_EventGraphStopped = "stopped";

static void * queue_puller(void * cbiv) {
    rage_EventCbInfo * cbi = cbiv;
    bool running = true;
    while (running) {
        rage_Event * evt = rage_queue_get_block(cbi->q);
        running = rage_event_type(evt) != rage_EventGraphStopped;
        cbi->cb(cbi->ctx, evt);
    }
    free(cbi);
    return NULL;
}

rage_Error rage_graph_start_processing(rage_Graph * g, rage_EventCb evt_cb, void * cb_ctx) {
    rage_Error err = rage_proc_block_start(g->pb);
    if (!RAGE_FAILED(err)) {
        err = rage_jack_backend_activate(g->jb);
    }
    if (RAGE_FAILED(err)) {
        rage_proc_block_stop(g->pb);
    } else {
        rage_EventCbInfo * cbi = malloc(sizeof(rage_EventCbInfo));
        cbi->q = g->evt_q;
        cbi->cb = evt_cb;
        cbi->ctx = cb_ctx;
        if (pthread_create(&g->q_thread, NULL, queue_puller, cbi)) {
            err = RAGE_ERROR("Failed to start queue thread");
            free(cbi);
            rage_jack_backend_deactivate(g->jb);
            rage_proc_block_stop(g->pb);
        }
    }
    return err;
}

void rage_graph_stop_processing(rage_Graph * g) {
    rage_jack_backend_deactivate(g->jb);
    rage_proc_block_stop(g->pb);
    rage_Event * evt = rage_event_new(
        rage_EventGraphStopped, NULL, NULL, NULL, NULL);
    rage_queue_put_block(g->evt_q, rage_queue_item_new(evt));
    pthread_join(g->q_thread, NULL);
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
        rage_Graph * g, rage_ElementType * cet,
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

rage_NewEventId rage_graph_update_node(
        rage_GraphNode * n, uint32_t const series_idx,
        rage_TimeSeries const ts) {
    return rage_harness_set_time_series(n->harness, series_idx, ts);
}

void rage_graph_remove_node(rage_Graph * g, rage_GraphNode * n) {
    rage_proc_block_unmount(n->harness);
    rage_element_free(n->elem);
    free(n);
}

static rage_Harness * rage_graph_get_harness(rage_GraphNode * n) {
    return (n == NULL) ? NULL : n->harness;
}

rage_ConTrans * rage_graph_con_trans_start(rage_Graph * g) {
    return rage_proc_block_con_trans_start(g->pb);
}

void rage_graph_con_trans_commit(rage_ConTrans * ct) {
    rage_proc_block_con_trans_commit(ct);
}

void rage_graph_con_trans_abort(rage_ConTrans * ct) {
    rage_proc_block_con_trans_abort(ct);
}

rage_Error rage_graph_connect(
        rage_ConTrans * ct,
        rage_GraphNode * source, uint32_t source_idx,
        rage_GraphNode * sink, uint32_t sink_idx) {
    return rage_proc_block_connect(
        ct,
        rage_graph_get_harness(source), source_idx,
        rage_graph_get_harness(sink), sink_idx);
}

rage_Error rage_graph_disconnect(
        rage_ConTrans * ct,
        rage_GraphNode * source, uint32_t source_idx,
        rage_GraphNode * sink, uint32_t sink_idx) {
    return rage_proc_block_disconnect(
        ct,
        rage_graph_get_harness(source), source_idx,
        rage_graph_get_harness(sink), sink_idx);
}
