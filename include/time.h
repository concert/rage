#pragma once

#include <stdint.h>

typedef struct {
    uint64_t second;
    uint32_t fraction;
} rage_Time;


float rage_time_delta(rage_Time a, rage_Time b);
