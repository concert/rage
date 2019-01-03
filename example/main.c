#include <stdio.h>
#include <unistd.h> // for sleep
#include "error.h"
#include "graph.h"
#include "loader.h"

typedef struct {
    rage_Graph * graph;
    rage_ElementLoader * el;
    rage_ElementKind * persist;
    rage_ElementKind * amp;
    rage_ConcreteElementType * st_persist;
    rage_ConcreteElementType * st_amp;
} example_Bits;

static void free_bits(example_Bits * bits) {
    if (bits->st_persist)
        rage_concrete_element_type_free(bits->st_persist);
    if (bits->st_amp)
        rage_concrete_element_type_free(bits->st_amp);
    if (bits->persist)
        rage_element_loader_unload(bits->persist);
    if (bits->amp)
        rage_element_loader_unload(bits->amp);
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
    // FIXME: loading story is poor
    rage_ElementKindLoadResult persist_ = rage_element_loader_load(
        "./build/libpersist.so");
    RAGE_ABORT_ON_FAILURE(persist_, 2);
    bits.persist = RAGE_SUCCESS_VALUE(persist_);
    rage_ElementKindLoadResult amp_ = rage_element_loader_load(
        "./build/libamp.so");
    RAGE_ABORT_ON_FAILURE(amp_, 2);
    bits.amp = RAGE_SUCCESS_VALUE(amp_);
    printf("Element kinds loaded\n");
    rage_Atom stereo[] = {
        {.i=2}
    };
    rage_Atom * stereop = stereo;
    rage_NewConcreteElementType st_persist_ = rage_element_type_specialise(
        bits.persist, &stereop);
    RAGE_ABORT_ON_FAILURE(st_persist_, 3);
    bits.st_persist = RAGE_SUCCESS_VALUE(st_persist_);
    rage_NewConcreteElementType st_amp_ = rage_element_type_specialise(
        bits.amp, &stereop);
    RAGE_ABORT_ON_FAILURE(st_amp_, 3);
    bits.st_amp = RAGE_SUCCESS_VALUE(st_amp_);
    // Persist params
    rage_Atom preroll[] = {
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
    rage_TimePoint persist_pts[] = {
        {
            .time = {.second = 0},
            .value = &(preroll[0]),
            .mode = RAGE_INTERPOLATION_CONST
        },
        {
            .time = {.second = 1},
            .value = &(recbit[0]),
            .mode = RAGE_INTERPOLATION_CONST
        },
        {
            .time = {.second = 10},
            .value = &(terminus[0]),
            .mode = RAGE_INTERPOLATION_CONST
        }
    };
    rage_TimeSeries persist_ts = {
        .len = 3,
        .items = persist_pts
    };
    // Amp params
    rage_Atom full_vol[] = {
        {.f = 1.0}
    };
    rage_Atom half_vol[] = {
        {.f = 0.1}
    };
    rage_TimePoint amp_pts[] = {
        {
            .time = {.second = 0},
            .value = full_vol,
            .mode = RAGE_INTERPOLATION_CONST
        },
        {
            .time = {.second = 6, .fraction = 0.1 * UINT32_MAX},
            .value = full_vol,
            .mode = RAGE_INTERPOLATION_LINEAR
        },
        {
            .time = {.second = 7},
            .value = half_vol,
            .mode = RAGE_INTERPOLATION_CONST
        }
    };
    rage_TimeSeries amp_ts = {
        .len = 3,
        .items = amp_pts
    };
    printf("Starting graph...\n");
    rage_Error en_st = rage_graph_start_processing(bits.graph);
    RAGE_ABORT_ON_FAILURE(en_st, 4);
    printf("Adding node...\n");
    rage_NewGraphNode new_node = rage_graph_add_node(
        bits.graph, bits.st_persist, &persist_ts);
    RAGE_ABORT_ON_FAILURE(new_node, 5);
    rage_GraphNode * pnode = RAGE_SUCCESS_VALUE(new_node);
    new_node = rage_graph_add_node(
        bits.graph, bits.st_amp, &amp_ts);
    RAGE_ABORT_ON_FAILURE(new_node, 6);
    rage_GraphNode * anode = RAGE_SUCCESS_VALUE(new_node);
    rage_ConTrans * ct = rage_graph_con_trans_start(bits.graph);
    for (uint32_t chan = 0; chan < 2; chan++) {
        rage_Error connect_result = rage_graph_connect(
            ct, pnode, chan, anode, chan);
        RAGE_ABORT_ON_FAILURE(connect_result, 7 + chan);
        connect_result = rage_graph_connect(
            ct, anode, chan, NULL, chan);
        RAGE_ABORT_ON_FAILURE(connect_result, 9 + chan);
    }
    rage_graph_con_trans_commit(ct);
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
    sleep(10);
    printf("Stopping...\n");
    rage_graph_remove_node(bits.graph, anode);
    rage_graph_remove_node(bits.graph, pnode);
    rage_graph_stop_processing(bits.graph);
    free_bits(&bits);
}
