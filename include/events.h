#pragma once

#include <stdint.h>
#include "atoms.h"
#include "time.h"

typedef struct {
    rage_Time time;
    rage_TupleDef * type;
    uint32_t size;
} rage_EventHeader;

typedef struct {
    void * buffer;
    uint16_t n_events;
    uint32_t size;
    uint32_t capacity;
} rage_EventFrame;
