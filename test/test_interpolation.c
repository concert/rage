#include <stdio.h>
#include "interpolation.h"

int check(
        rage_Interpolator interpolator, rage_TimeSeries ts, uint32_t second,
        float expected) {
    float got = rage_interpolate(interpolator, (rage_Time) {.second=second}, ts)[0].f;
    if (got != expected) {
        printf("%u: %f != %f", second, got, expected);
        return 1;
    }
    return 0;
}

int checks(rage_Interpolator interpolator, rage_TimeSeries ts) {
    if (check(interpolator, ts, 0, 0.0))
        return 1;
    if (check(interpolator, ts, 1, 0.5))
        return 1;
    if (check(interpolator, ts, 2, 1.0))
        return 1;
    if (check(interpolator, ts, 3, 1.0))
        return 1;
    if (check(interpolator, ts, 4, 2.0))
        return 1;
    if (check(interpolator, ts, 5, 2.0))
        return 1;
    return 0;
}

int main() {
    rage_FieldDef fields[] = {
        {
            .name = "f_field",
            .type = {.type = RAGE_ATOM_FLOAT, .name = "float", .constraints = {}}
        }
    };
    rage_TupleDef td = {
        .name = "Interpolation Test",
        .description = "Testing's great",
        .len = 1,
        .items = fields
    };
    rage_Atom vals[] = {{.f = 0}, {.f = 1}, {.f = 2}};
    rage_TimePoint tps[] = {
        {
            .time = {.second = 0},
            .value = &(vals[0]),
            .mode = RAGE_INTERPOLATION_LINEAR
        },
        {
            .time = {.second = 2},
            .value = &(vals[1]),
            .mode = RAGE_INTERPOLATION_CONST
        },
        {
            .time = {.second = 4},
            .value = &(vals[2]),
            .mode = RAGE_INTERPOLATION_CONST
        }
    };
    rage_TimeSeries ts = {
        .len = 3,
        .items = tps
    };
    rage_InitialisedInterpolator ii = rage_interpolator_new(td);
    if (RAGE_FAILED(ii)) {
        printf("Interpolator init failed: %s", RAGE_FAILURE_VALUE(ii));
        return 2;
    }
    rage_Interpolator interpolator = RAGE_SUCCESS_VALUE(ii);
    int rv = checks(interpolator, ts);
    rage_interpolator_free(interpolator);
    return rv;
}
