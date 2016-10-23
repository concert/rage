#include "time.h"

float rage_time_delta(rage_Time a, rage_Time b) {
    // FIXME: precision, fraction
    return (float) a.second - b.second;
}
