#pragma once

#include "atoms.h"
#include "interpolation.h"

typedef enum {
    RAGE_STREAM_AUDIO
} rage_StreamDef;

typedef RAGE_ARRAY(rage_TupleDef const) rage_ProcessRequirementsControls;
typedef RAGE_ARRAY(rage_StreamDef const) rage_ProcessRequirementsStreams;
typedef struct {
    rage_ProcessRequirementsControls controls;
    rage_ProcessRequirementsStreams inputs;
    rage_ProcessRequirementsStreams outputs;
} rage_ProcessRequirements;

typedef float const * rage_InStream;
typedef float * rage_OutStream;

typedef struct {
    rage_TransportState * transport;
    rage_Interpolator ** controls;
    rage_InStream * inputs;
    rage_OutStream * outputs;
} rage_Ports;

rage_Ports rage_ports_new(rage_ProcessRequirements const * const requirements);
void rage_ports_free(rage_Ports ports);
