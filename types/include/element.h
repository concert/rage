#pragma once

#include <stdint.h>
#include "atoms.h"
#include "ports.h"
#include "error.h"
#include "interpolation.h"

typedef struct rage_ElementState rage_ElementState;
typedef struct rage_ElementTypeState rage_ElementTypeState;

// Forward declaration:
typedef struct rage_ElementType rage_ElementType;
typedef rage_Error (*rage_KindSpecialise)(
    rage_ElementType * type, rage_Atom ** params);

typedef RAGE_OR_ERROR(rage_ElementState *) rage_NewElementState;
typedef rage_NewElementState (*rage_ElementStateNew)(
    rage_ElementTypeState * type_state, uint32_t sample_rate);
typedef void (*rage_ElementTypeDestroy)(rage_ElementType * type);
typedef void (*rage_ElementStateFree)(rage_ElementState * state);
typedef void (*rage_ElementProcess)(
    rage_ElementState * state, rage_TransportState const transport_state,
    uint32_t period_size, rage_Ports const * ports);

typedef rage_Error (*rage_ElementPrepare)(
    rage_ElementState * state, rage_InterpolatedView ** controls);
typedef rage_Error (*rage_ElementClear)(
    rage_ElementState * state, rage_InterpolatedView ** controls,
    rage_FrameNo const to_present);
typedef rage_Error (*rage_ElementClean)(
    rage_ElementState * state, rage_InterpolatedView ** controls);

typedef RAGE_ARRAY(rage_TupleDef const) rage_ParamDefList;

typedef struct {
    rage_ParamDefList const * const parameters;
    rage_KindSpecialise const specialise;
} rage_ElementKind;

struct rage_ElementType {
    // Mandatory:
    RAGE_EMBED_STRUCT(rage_InstanceSpec, spec);
    rage_ElementTypeDestroy type_destroy;
    rage_ElementStateNew state_new;
    rage_ElementProcess process;
    rage_ElementStateFree state_free;
    // Optional:
    rage_ElementPrepare prep;
    rage_ElementClear clear;  // Required if prep provided
    rage_ElementClean clean;
    rage_ElementTypeState * type_state;
};
