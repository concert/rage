#pragma once
#include <stdint.h>
#include "macros.h"

typedef struct rage_BackendState rage_BackendState;

typedef void (*rage_BufferGet)(
    rage_BackendState * state, uint32_t ext_revision,
    uint32_t n_frames, void ** inputs, void ** outputs);

// FIXME: Rename to ensure_tick or something?
typedef struct rage_TickForcing rage_TickForcing;
typedef rage_TickForcing * (*rage_TickForceStart)(rage_BackendState * b);
typedef void (*rage_TickForceEnd)(rage_TickForcing * tf);

typedef struct {
    rage_TickForceStart tick_force_start;
    rage_TickForceEnd tick_force_end;
    rage_BufferGet get_buffers;
    rage_BackendState * b;
} rage_BackendHooks;

typedef void (*rage_ProcessCb)(uint32_t n_frames, void * data);
typedef void (*rage_SetExternalsCb)(
    void * data, uint32_t const ext_revision,
    uint32_t const n_ins, uint32_t const n_outs);

typedef rage_BackendHooks (*rage_BackendSetupProcess)(
    rage_BackendState * bs,
    void * data,
    rage_ProcessCb process,
    rage_SetExternalsCb set_externals,
    uint32_t * sample_rate,
    uint32_t * buffer_size);

typedef struct {
    rage_BackendState * state;
    rage_BackendSetupProcess setup_process;
} rage_BackendInterface;

// FIXME: Should this be allowed to fail?
rage_BackendHooks rage_backend_setup_process(
    rage_BackendInterface * bi,
    void * data,
    void (*process)(uint32_t n_frames, void * data),
    void (*set_externals)(
        void * data, uint32_t const ext_revision,
        uint32_t const n_ins, uint32_t const n_outs),
    uint32_t * sample_rate,
    uint32_t * buffer_size);

// FIXME: Rest of file looking increasingly strange and pointless:
typedef RAGE_ARRAY(char *) rage_BackendDescriptions;

typedef struct {
    rage_BackendDescriptions inputs;
    rage_BackendDescriptions outputs;
} rage_BackendPorts;

typedef struct {
    uint32_t sample_rate;
    uint32_t buffer_size;
    rage_BackendPorts ports;
} rage_BackendConfig;
