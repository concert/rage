#pragma once
#include "error.h"
#include "atoms.h"
#include "loader.h"
#include "binding_interface.h"
#include "con_trans.h"
#include "event.h"

typedef void (*rage_EventCb)(void * ctx, rage_Event * evt);

extern rage_EventType * rage_EventGraphStopped;

typedef struct rage_Graph rage_Graph;
typedef RAGE_OR_ERROR(rage_Graph *) rage_NewGraph;
rage_NewGraph rage_graph_new(rage_BackendPorts ports, uint32_t sample_rate);
void rage_graph_free(rage_Graph * g);

rage_Error rage_graph_start_processing(rage_Graph * g, rage_EventCb evt_cb, void * cb_ctx);
void rage_graph_stop_processing(rage_Graph * g);

void rage_graph_set_transport_state(rage_Graph * g, rage_TransportState s);
rage_Error rage_graph_transport_seek(rage_Graph * g, rage_Time const * target);

typedef struct rage_GraphNode rage_GraphNode;
typedef RAGE_OR_ERROR(rage_GraphNode *) rage_NewGraphNode;
rage_NewGraphNode rage_graph_add_node(
        rage_Graph * g, rage_ConcreteElementType * cet,
        rage_TimeSeries const * ts);
rage_NewEventId rage_graph_update_node(
        rage_GraphNode * n, uint32_t const series_idx,
        rage_TimeSeries const ts);
void rage_graph_remove_node(rage_Graph * g, rage_GraphNode * n);

rage_ConTrans * rage_graph_con_trans_start(rage_Graph * g);
void rage_graph_con_trans_commit(rage_ConTrans * ct);
void rage_graph_con_trans_abort(rage_ConTrans * ct);
rage_Error rage_graph_connect(
    rage_ConTrans * ct,
    rage_GraphNode * source, uint32_t source_idx,
    rage_GraphNode * sink, uint32_t sink_idx);
rage_Error rage_graph_disconnect(
    rage_ConTrans * ct,
    rage_GraphNode * source, uint32_t source_idx,
    rage_GraphNode * sink, uint32_t sink_idx);
