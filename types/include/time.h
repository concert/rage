#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t second;
    uint32_t fraction;
} rage_Time;

float rage_time_delta(rage_Time a, rage_Time b);
bool rage_time_after(rage_Time a, rage_Time b);
rage_Time rage_time_sample_length(uint32_t const sample_rate);
rage_Time rage_time_add(rage_Time const a, rage_Time const b);
