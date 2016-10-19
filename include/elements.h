#pragma once

#include "atoms.h"
#include "ports.h"
#include "error.h"
#include "macros.h"

typedef rage_PortDescription * (*rage_ElementGetPortsDescription)(rage_Tuple params);
typedef void * (*rage_ElementStateNew)(rage_Tuple params);
typedef void (*rage_ElementStateFree)(void * state);
typedef rage_Error (*rage_ElementPrepare)(void * state, rage_Tuple params);
typedef void (*rage_ElementHalt)(void * state);
// Maybe break up the stream and event ports more explicitly
typedef rage_Error (*rage_ElementRun)(void * state, rage_Time time, unsigned nsamples, rage_InPort * inputs, rage_OutPort * outputs);

typedef struct {
    rage_TupleDef parameters;
    // rage_ElementGetPorts get_ports;
    rage_ElementStateNew state_new;
    rage_ElementPrepare prepare;
    rage_ElementRun run;
    rage_ElementHalt halt;
    rage_ElementStateFree state_free;
} rage_ElementType;

typedef enum {
    RAGE_ELEMENT_PHASE_INIT,
    RAGE_ELEMENT_PHASE_PREPARED,
    RAGE_ELEMENT_PHASE_HALTED
} rage_ElementPhase;

typedef struct {
    rage_ElementType * type;
    rage_ElementPhase phase;
    void * state;
} rage_Element;

typedef void * rage_ElementLoader;

rage_ElementLoader rage_element_loader_new();
void rage_element_loader_free(rage_ElementLoader el);

typedef RAGE_ARRAY(char *) rage_ElementTypes;
rage_ElementTypes rage_element_loader_list(rage_ElementLoader el);
rage_ElementType * rage_element_loader_load(rage_ElementLoader el, char const * type_name);
void rage_element_loader_unload(rage_ElementLoader el, rage_ElementType * type);

rage_Element * rage_element_new(rage_ElementType * type, rage_Tuple params);
void rage_element_free(rage_Element * elem);
rage_Error rage_element_prepare(rage_Element * elem, rage_Tuple params);
void rage_element_halt(rage_Element * elem);
void rage_element_free(rage_Element * elem);

// rage_PortsDescription rage_element_get_ports(rage_Element const * const elem);
