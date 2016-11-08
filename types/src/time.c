#include "time.h"

const static float frac_float = 1.0 / UINT32_MAX;

static float rage_time_as_float(rage_Time t) {
    return t.second + (t.fraction * frac_float);
}

float rage_time_delta(rage_Time a, rage_Time b) {
    return rage_time_as_float(a) - rage_time_as_float(b);
}

bool rage_time_after(rage_Time a, rage_Time b) {
    if (a.second > b.second)
        return true;
    if (a.second < b.second)
        return false;
    return a.fraction > b.fraction;
}

rage_Time rage_time_add(rage_Time const a, rage_Time const b) {
    uint64_t second = a.second + b.second;
    uint64_t fraction = a.fraction + b.fraction;
    if (fraction > UINT32_MAX) {
        second++;
        fraction -= UINT32_MAX;
    }
    return (rage_Time) {.second=second, .fraction=fraction};
}
