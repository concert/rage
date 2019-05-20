#pragma once
#include <stdint.h>
#include <macros.h>
#include <error.h>

typedef struct rage_BackendState rage_BackendState;

typedef void (*rage_BufferGet)(
    rage_BackendState * state, uint32_t ext_revision,
    uint32_t n_frames, void ** inputs, void ** outputs);

typedef struct rage_Ticking rage_Ticking;
typedef rage_Ticking * (*rage_TickEnsureStart)(rage_BackendState * b);
typedef void (*rage_TickEnsureEnd)(rage_Ticking * tf);

typedef struct {
    rage_TickEnsureStart tick_ensure_start;
    rage_TickEnsureEnd tick_ensure_end;
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
    uint32_t * buffer_size);

typedef struct {
    rage_BackendState * state;
    rage_BackendSetupProcess setup_process;
    void (*unset_process)(rage_BackendState * state);
    rage_Error (*activate)(rage_BackendState * state);
    rage_Error (*deactivate)(rage_BackendState * state);
    uint32_t const * sample_rate;
} rage_BackendInterface;

uint32_t rage_backend_get_sample_rate(rage_BackendInterface * bi);

// FIXME: Should this be allowed to fail?
rage_BackendHooks rage_backend_setup_process(
    rage_BackendInterface * bi,
    void * data,
    void (*process)(uint32_t n_frames, void * data),
    void (*set_externals)(
        void * data, uint32_t const ext_revision,
        uint32_t const n_ins, uint32_t const n_outs),
    uint32_t * buffer_size);
// FIXME: Should this be allowed to fail?
void rage_backend_unset_process(rage_BackendInterface * bi);

rage_Error rage_backend_activate(rage_BackendInterface * bi);
rage_Error rage_backend_deactivate(rage_BackendInterface * bi);
