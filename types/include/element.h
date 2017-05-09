#pragma once

#include <stdint.h>
#include "atoms.h"
#include "ports.h"
#include "error.h"
#include "interpolation.h"

typedef RAGE_OR_ERROR(void *) rage_NewElementState;
typedef rage_NewElementState (*rage_ElementStateNew)(
    uint32_t sample_rate, uint32_t frame_size, rage_Atom ** params);
typedef void (*rage_ElementStateFree)(void * state);
typedef rage_ProcessRequirements (*rage_ElementGetPortsDescription)(rage_Atom ** params);
// FIXME: below can fail
typedef void (*rage_ElementFreePortsDescription)(rage_ProcessRequirements);
typedef rage_Error (*rage_ElementProcess)(
    void * state, rage_TransportState const transport_state,
    rage_Ports const * ports);

typedef RAGE_OR_ERROR(uint32_t) rage_PreparedFrames;
typedef rage_PreparedFrames (*rage_ElementPrepare)(
    void * state, rage_InterpolatedView ** controls);
typedef rage_Error (*rage_ElementClear)(
    void * state, rage_InterpolatedView ** controls, rage_FrameNo const preserve);
typedef rage_PreparedFrames (*rage_ElementClean)(
    void * state, rage_InterpolatedView ** controls);

typedef RAGE_ARRAY(rage_TupleDef const) rage_ParamDefList;

typedef struct {
    // Mandatory:
    rage_ParamDefList const * const parameters;
    rage_ElementGetPortsDescription const get_ports;
    rage_ElementFreePortsDescription const free_ports;
    rage_ElementStateNew const state_new;
    rage_ElementProcess const process;
    rage_ElementStateFree const state_free;
    // Optional:
    rage_ElementPrepare const prep;
    rage_ElementClear const clear;  // Required if prep provided
    rage_ElementClean const clean;
} rage_ElementType;
