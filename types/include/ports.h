#pragma once

#include "atoms.h"
#include "interpolation.h"
#include "transport_state.h"

/*
 * Stream definition types.
 */
typedef enum {
    RAGE_STREAM_AUDIO
} rage_StreamDef;

typedef RAGE_ARRAY(rage_TupleDef const) rage_InstanceSpecControls;
typedef RAGE_ARRAY(rage_StreamDef const) rage_InstanceSpecStreams;
typedef struct {
    rage_InstanceSpecControls controls;
    rage_InstanceSpecStreams inputs;
    rage_InstanceSpecStreams outputs;
    uint32_t max_uncleaned_frames;
    uint32_t max_period_size;
    uint32_t n_periods_latency;
} rage_InstanceSpec;

// FIXME: typedeffed pointers
typedef float const * rage_InStream;
typedef float * rage_OutStream;

typedef struct {
    rage_InterpolatedView ** controls;
    rage_InStream * inputs;
    rage_OutStream * outputs;
} rage_Ports;

rage_Ports rage_ports_new(rage_InstanceSpec const * const spec);
void rage_ports_free(rage_Ports ports);
