#pragma once

#include "atoms.h"
#include "ports.h"
#include "error.h"

typedef rage_PortDescription * (*rage_ElementGetPortsDescription)(rage_Tuple params);
typedef void * (*rage_ElementStateNew)(rage_Tuple params);
typedef void (*rage_ElementStateFree)(void * state);
typedef void (*rage_ElementPrepare)(void * state, rage_Tuple params);
typedef void (*rage_ElementHalt)(void * state);
// Maybe break up the stream and event ports more explicitly
typedef rage_Error (*rage_ElementRun)(void * state, rage_Time time, unsigned nsamples, rage_InPort * inputs, rage_OutPort * outputs);
