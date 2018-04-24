#pragma once

#include <stdint.h>
#include <stdbool.h>

/*
 * NTP inspired timestamp format (but with a bigger int to avoid the impending
 * overflow).
 */
typedef struct {
    uint64_t second;
    uint32_t fraction;
} rage_Time;

/*
 * Session time frame number.
 */
typedef uint64_t rage_FrameNo;

/*
 * Returns true if b is later than a.
 */
bool rage_time_after(rage_Time a, rage_Time b);
/*
 * Calculate the length of a frame from the sample rate.
 */
rage_Time rage_time_sample_length(uint32_t const sample_rate);
/*
 * Calculate the frame number corresponding to a time at a sample rate.
 */
rage_FrameNo rage_time_to_frame(rage_Time const * t, uint32_t sample_rate);

/*
 * Add times.
 */
rage_Time rage_time_add(rage_Time const a, rage_Time const b);
