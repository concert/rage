#pragma once
#include <stdint.h>
#include "macros.h"

typedef struct rage_BackendState rage_BackendState;

typedef struct {
    rage_BackendState * state;
    void (*get_buffers)(
        rage_BackendState * state, uint32_t ext_revision,
        uint32_t n_frames, void ** inputs, void ** outputs);
} rage_BackendInterface;

typedef RAGE_ARRAY(char *) rage_BackendDescriptions;

typedef struct {
    rage_BackendDescriptions inputs;
    rage_BackendDescriptions outputs;
} rage_BackendPorts;

typedef struct {
    uint32_t sample_rate;
    uint32_t buffer_size;
    rage_BackendPorts ports;
    void (*process)(rage_BackendInterface const * b, uint32_t n_frames, void * data);
    void * data;
} rage_BackendConfig;

void rage_backend_get_buffers(
    rage_BackendInterface const * b, uint32_t revision,
    uint32_t n_frames, void ** inputs, void ** outputs);
