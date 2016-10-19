#pragma once

#include <stdbool.h>
#include "events.h"

typedef enum {
    RAGE_PORT_AUDIO,
    RAGE_PORT_EVENT
} rage_PortType;

typedef struct port_desc {
    char * name;
    char * doc;
    bool input;
    rage_PortType type;
    struct port_desc * next;
} rage_PortDescription;

typedef union {
    float const * const samples;
    rage_EventFrame const * const events;
} rage_InPort;

typedef union {
    float * const samples;
    rage_EventFrame * const events;
} rage_OutPort;
