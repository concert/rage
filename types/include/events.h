#pragma once

#include <stdint.h>
#include "interpolation.h"

typedef struct {
    rage_TimeSeries events;
    char * buffer;
    uint32_t fill;
    uint32_t capacity;
} rage_EventFrame;

rage_EventFrame * rage_event_frame_new(uint32_t capacity);
void rage_event_frame_free(rage_EventFrame * const frame);
