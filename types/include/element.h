#pragma once

#include <stdint.h>
#include "atoms.h"
#include "ports.h"
#include "error.h"
#include "interpolation.h"

typedef struct rage_ElementState rage_ElementState;

typedef RAGE_OR_ERROR(rage_ElementState *) rage_NewElementState;
typedef rage_NewElementState (*rage_ElementStateNew)(
    uint32_t sample_rate, rage_Atom ** params);
typedef void (*rage_ElementStateFree)(rage_ElementState * state);
typedef RAGE_OR_ERROR(rage_InstanceSpec) rage_NewInstanceSpec;
typedef rage_NewInstanceSpec (*rage_ElementGetPortsDescription)(rage_Atom ** params);
typedef void (*rage_ElementFreePortsDescription)(rage_InstanceSpec);
typedef void (*rage_ElementProcess)(
    rage_ElementState * state, rage_TransportState const transport_state,
    uint32_t period_size, rage_Ports const * ports);

typedef rage_Error (*rage_ElementPrepare)(
    rage_ElementState * state, rage_InterpolatedView ** controls);
typedef rage_Error (*rage_ElementClear)(
    rage_ElementState * state, rage_InterpolatedView ** controls, rage_FrameNo const to_present);
typedef rage_Error (*rage_ElementClean)(
    rage_ElementState * state, rage_InterpolatedView ** controls);

typedef RAGE_ARRAY(rage_TupleDef const) rage_ParamDefList;

typedef struct {
    // Mandatory:
    rage_ParamDefList const * const parameters;
    rage_ElementGetPortsDescription const ports_get;
    rage_ElementFreePortsDescription const ports_free;
    rage_ElementStateNew const state_new;
    rage_ElementProcess const process;
    rage_ElementStateFree const state_free;
    // Optional:
    rage_ElementPrepare const prep;
    rage_ElementClear const clear;  // Required if prep provided
    rage_ElementClean const clean;
} rage_ElementKind;
