#pragma once

#include <stdint.h>
#include "atoms.h"
#include "ports.h"
#include "error.h"
#include "interpolation.h"

typedef RAGE_OR_ERROR(void *) rage_NewElementState;
typedef rage_NewElementState (*rage_ElementStateNew)(
    uint32_t sample_rate, uint32_t frame_size, rage_Atom * params);
typedef void (*rage_ElementStateFree)(void * state);
typedef rage_ProcessRequirements (*rage_ElementGetPortsDescription)(rage_Atom * params);
typedef void (*rage_ElementFreePortsDescription)(rage_ProcessRequirements);
typedef rage_Error (*rage_ElementProcess)(void * state, rage_Ports const * ports);
