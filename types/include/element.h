#pragma once

#include "atoms.h"
#include "ports.h"
#include "error.h"

typedef rage_PortDescription * (*rage_ElementGetPortsDescription)(rage_Tuple params);
typedef RAGE_OR_ERROR(void *) rage_NewElementState;
typedef rage_NewElementState (*rage_ElementStateNew)(rage_Tuple params);
typedef void (*rage_ElementStateFree)(void * state);
typedef rage_Error (*rage_ElementProcess)(
    void * state, rage_Time time, unsigned nsamples, rage_Port * ports);
