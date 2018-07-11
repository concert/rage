#pragma once
#include "error.h"
#include "atoms.h"
#include "loader.h"

typedef struct rage_Graph rage_Graph;
typedef RAGE_OR_ERROR(rage_Graph *) rage_NewGraph;
rage_NewGraph rage_graph_new(uint32_t sample_rate);
void rage_graph_free(rage_Graph * g);

rage_Error rage_graph_start_processing(rage_Graph * g);
void rage_graph_stop_processing(rage_Graph * g);
void rage_graph_set_transport_state(rage_Graph * g, rage_TransportState s);
rage_Error rage_graph_transport_seek(rage_Graph * g, rage_Time const * target);

typedef struct rage_GraphNode rage_GraphNode;
typedef RAGE_OR_ERROR(rage_GraphNode *) rage_NewGraphNode;
rage_NewGraphNode rage_graph_add_node(
        rage_Graph * g, rage_ConcreteElementType * cet,
        rage_TimeSeries const * ts);
void rage_graph_remove_node(rage_Graph * g, rage_GraphNode * n);
