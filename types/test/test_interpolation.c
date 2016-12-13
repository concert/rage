#include <stdio.h>
#include "interpolation.h"

static int check(
        rage_Interpolator * interpolator, rage_FrameNo consume,
        float expected) {
    rage_InterpolatedValue val = rage_interpolate(interpolator, consume);
    float got = val.value[0].f;
    if (got != expected) {
        printf("%f != %f", got, expected);
        return 1;
    }
    return 0;
}

static int checks(rage_Interpolator * interpolator) {
    if (check(interpolator, 0, 0.0))
        return 1;
    if (check(interpolator, 1, 0.5))
        return 1;
    if (check(interpolator, 1, 1.0))
        return 1;
    if (check(interpolator, 1, 1.0))
        return 1;
    if (check(interpolator, 1, 2.0))
        return 1;
    if (check(interpolator, 1, 2.0))
        return 1;
    return 0;
}

int main() {
    rage_AtomDef const unconstrained_float = {
        .type = RAGE_ATOM_FLOAT, .name = "float", .constraints = {}
    };
    rage_FieldDef fields[] = {
        {.name = "f_field", .type = &unconstrained_float}
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
    rage_TransportState transport = RAGE_TRANSPORT_ROLLING;
    rage_InitialisedInterpolator ii = rage_interpolator_new(
        &td, &ts, 1, &transport);
    if (RAGE_FAILED(ii)) {
        printf("Interpolator init failed: %s", RAGE_FAILURE_VALUE(ii));
        return 2;
    }
    rage_Interpolator * interpolator = RAGE_SUCCESS_VALUE(ii);
    int rv = checks(interpolator);
    rage_interpolator_free(&td, interpolator);
    return rv;
}
