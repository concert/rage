#include <stdio.h>
#include "interpolation.h"

static rage_InitialisedInterpolator interpolator_for(
        rage_TupleDef const * td, rage_TimePoint * points,
        unsigned n_points, uint8_t n_views) {
    rage_TimeSeries ts = {
        .len = n_points,
        .items = points
    };
    return rage_interpolator_new(td, &ts, 1, n_views);
}

static int check_with_single_field_interpolator(
        rage_AtomDef const * atom_def, rage_TimePoint * points,
        unsigned n_points, int (*checker)(rage_InterpolatedView *)) {
    rage_FieldDef fields[] = {
        {.name = "field", .type = atom_def}
    };
    rage_TupleDef td = {
        .name = "Interpolation Test",
        .description = "Testing's great",
        .len = 1,
        .items = fields
    };
    rage_InitialisedInterpolator ii = interpolator_for(
        &td, points, n_points, 1);
    if (RAGE_FAILED(ii)) {
        printf("Interpolator init failed: %s", RAGE_FAILURE_VALUE(ii));
        return 2;
    }
    rage_Interpolator * interpolator = RAGE_SUCCESS_VALUE(ii);
    int rv = checker(rage_interpolator_get_view(interpolator, 0));
    rage_interpolator_free(&td, interpolator);
    return rv;
}

#define RAGE_EQUALITY_CHECK(type, member, fmt) \
    static int member##_check( \
            rage_InterpolatedView * v, type expected) { \
        rage_InterpolatedValue const * val = rage_interpolated_view_value(v); \
        type got = val->value[0].member; \
        if (got != expected) { \
            printf(fmt " != " fmt, got, expected); \
            return 1; \
        } \
        return 0; \
    }

RAGE_EQUALITY_CHECK(float, f, "%f")

static int float_checks(rage_InterpolatedView * v) {
    if (f_check(v, 0.0))
        return 1;
    rage_interpolated_view_advance(v, 1);
    if (f_check(v, 0.5))
        return 1;
    rage_interpolated_view_advance(v, 1);
    if (f_check(v, 1.0))
        return 1;
    rage_interpolated_view_advance(v, 1);
    if (f_check(v, 1.0))
        return 1;
    rage_interpolated_view_advance(v, 1);
    if (f_check(v, 2.0))
        return 1;
    rage_interpolated_view_advance(v, 1);
    if (f_check(v, 2.0))
        return 1;
    rage_interpolated_view_advance(v, 1);
    return 0;
}

static rage_AtomDef const unconstrained_float = {
    .type = RAGE_ATOM_FLOAT, .name = "float", .constraints = {}};

static int interpolator_float_test() {
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
    return check_with_single_field_interpolator(
        &unconstrained_float, tps, 3, float_checks);
}

RAGE_EQUALITY_CHECK(uint32_t, frame_no, "%u")

static int time_checks(rage_InterpolatedView * v) {
    if (frame_no_check(v, 0))
        return 1;
    rage_interpolated_view_advance(v, 1);
    if (frame_no_check(v, 1))
        return 1;
    rage_interpolated_view_advance(v, 2);
    if (frame_no_check(v, 3))
        return 1;
    rage_interpolated_view_advance(v, 1);
    if (frame_no_check(v, 1))
        return 1;
    rage_interpolated_view_advance(v, 120);
    if (frame_no_check(v, 121))
        return 1;
    return 0;
}

static int interpolator_time_test() {
    rage_AtomDef const unconstrained_time = {
        .type = RAGE_ATOM_TIME, .name = "time", .constraints = {}};
    rage_Atom vals[] = {
        {.t = {.second=0}},
        {.t = {.second=1}}
    };
    rage_TimePoint tps[] = {
        {
            .time = {.second = 0},
            .value = &(vals[0]),
            .mode = RAGE_INTERPOLATION_CONST
        },
        {
            .time = {.second = 4},
            .value = &(vals[1]),
            .mode = RAGE_INTERPOLATION_CONST
        }
    };
    return check_with_single_field_interpolator(
        &unconstrained_time, tps, 2, time_checks);
}

static int interpolator_immediate_change_test() {
    rage_Atom val = {.f = 0};
    rage_TimePoint tps[] = {
        {
            .time = {.second = 0},
            .value = &val,
            .mode = RAGE_INTERPOLATION_CONST
        },
    };
    // FIXME: following 2 are same as in check_with_single_field
    rage_FieldDef fields[] = {
        {.name = "field", .type = &unconstrained_float}
    };
    rage_TupleDef td = {
        .name = "Interpolation Test",
        .description = "Testing's great",
        .len = 1,
        .items = fields
    };
    rage_InitialisedInterpolator ii = interpolator_for(&td, tps, 1, 1);
    if (RAGE_FAILED(ii)) {
        return 2;
    }
    int rval = 0;
    rage_Interpolator * interpolator = RAGE_SUCCESS_VALUE(ii);
    rage_InterpolatedView * iv = rage_interpolator_get_view(interpolator, 0);
    rage_InterpolatedValue const * obtained = rage_interpolated_view_value(iv);
    // TODO: checks on valid_for
    if (obtained->value[0].f != val.f) {
        rval += 1;
    } else {
        val.f = 1;
        rage_TimeSeries ts = {
            .len = 1,
            .items = tps
        };
        rage_Finaliser * change_complete = rage_interpolator_change_timeseries(
            interpolator, &ts, 0);
        obtained = rage_interpolated_view_value(iv);
        rage_finaliser_wait(change_complete);
        if (obtained->value[0].f != val.f) {
            rval += 2;
        }
    }
    rage_interpolator_free(&td, interpolator);
    return rval;
}

// TODO: multiple views test

// FIXME: use normal test main
int main() {
    int i = interpolator_float_test();
    if (i)
        return i;
    i = interpolator_time_test();
    if (i)
        return i;
    return interpolator_immediate_change_test();
}
