#include <stdio.h>
#include "interpolation.h"

static int check(
        rage_InterpolatedView * v, float expected) {
    rage_InterpolatedValue const * val = rage_interpolated_view_value(v);
    float got = val->value[0].f;
    if (got != expected) {
        printf("%f != %f", got, expected);
        return 1;
    }
    return 0;
}

static int checks(rage_Interpolator * interpolator) {
    // FIXME: this is a bit of a hack to make it go
    rage_InterpolatedView * v = rage_interpolator_get_view(interpolator, 0);
    if (check(v, 0.0))
        return 1;
    rage_interpolated_view_advance(v, 1);
    if (check(v, 0.5))
        return 1;
    rage_interpolated_view_advance(v, 1);
    if (check(v, 1.0))
        return 1;
    rage_interpolated_view_advance(v, 1);
    if (check(v, 1.0))
        return 1;
    rage_interpolated_view_advance(v, 1);
    if (check(v, 2.0))
        return 1;
    rage_interpolated_view_advance(v, 1);
    if (check(v, 2.0))
        return 1;
    rage_interpolated_view_advance(v, 1);
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
    rage_InitialisedInterpolator ii = rage_interpolator_new(
        &td, &ts, 1, 1);
    if (RAGE_FAILED(ii)) {
        printf("Interpolator init failed: %s", RAGE_FAILURE_VALUE(ii));
        return 2;
    }
    rage_Interpolator * interpolator = RAGE_SUCCESS_VALUE(ii);
    int rv = checks(interpolator);
    rage_interpolator_free(&td, interpolator);
    return rv;
}
