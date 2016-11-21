#pragma once

#include <stdbool.h>
#include "events.h"

typedef enum {
    RAGE_PORT_STREAM,
    RAGE_PORT_EVENT
} rage_PortType;

typedef enum {
    RAGE_STREAM_AUDIO
} rage_StreamPortDef;

typedef rage_TupleDef rage_EventPortDef;

typedef struct port_desc {
    bool is_input;
    rage_PortType type;
    union {
        rage_StreamPortDef stream_def;
        rage_EventPortDef event_def;
    };
    struct port_desc * next;
} rage_PortDescription;

rage_PortDescription * rage_port_description_copy(rage_PortDescription pd);
void rage_port_description_free(rage_PortDescription * const pd);
uint32_t rage_port_description_count(rage_PortDescription const * pd);

typedef union {
    float const * samples;
    rage_EventFrame const * events;
} rage_InPort;

typedef union {
    float * samples;
    rage_EventFrame * events;
} rage_OutPort;

typedef struct {
    rage_PortDescription const * desc;
    union {
        rage_InPort in;
        rage_OutPort out;
    };
} rage_Port;
