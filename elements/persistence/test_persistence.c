#include "graph_test_factories.h"
#include "loader.h"
#include "ports.h"
#include "error.h"
#include "testing.h"

static void new_noisy_out_buffers(
        rage_Ports * ports, uint32_t n_outs, uint32_t frame_size) {
    for (uint32_t i = 0; i < n_outs; i++) {
        ports->outputs[i] = malloc(frame_size * sizeof(float));
        for (uint32_t j = 0; j < frame_size; j++) {
            ports->outputs[i][j] = ((j % 100) - 50) / 50;  // Saw
        }
    }
}

static void free_out_buffers(rage_Ports * ports, uint32_t n_outs) {
    for (uint32_t i = 0; i < n_outs; i++) {
        free((void *) ports->outputs[i]);
    }
}

static rage_Error check_outs_silent(
        rage_Ports * ports, uint32_t n_outs, uint32_t frame_size) {
    for (uint32_t i = 0; i < n_outs; i++) {
        for (uint32_t j = 0; j < frame_size; j++) {
            if (ports->outputs[i][j] != 0.0)
                return RAGE_ERROR("Not silent");
        }
    }
    return RAGE_OK;
}

static rage_Error test_pause_silent() {
    rage_NewTestElem nte = new_test_elem("./libpersist.so");
    if (RAGE_FAILED(nte)) {
        return RAGE_FAILURE_CAST(rage_Error, nte);
    }
    rage_TestElem te = RAGE_SUCCESS_VALUE(nte);
    rage_Ports ports = rage_ports_new(&te.type->spec);
    new_noisy_out_buffers(&ports, te.type->spec.outputs.len, te.elem_frame_size);
    rage_element_process(te.elem, RAGE_TRANSPORT_STOPPED, te.elem_frame_size, &ports);
    rage_Error rv = check_outs_silent(&ports, te.type->spec.outputs.len, te.elem_frame_size);
    free_out_buffers(&ports, te.type->spec.outputs.len);
    rage_ports_free(ports);
    free_test_elem(te);
    return rv;
}

TEST_MAIN(test_pause_silent)
