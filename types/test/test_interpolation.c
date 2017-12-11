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

static rage_Error check_with_single_field_interpolator(
        rage_AtomDef const * atom_def, rage_TimePoint * points,
        unsigned n_points, rage_Error (*checker)(rage_Interpolator *),
        uint8_t n_views) {
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
        &td, points, n_points, n_views);
    if (RAGE_FAILED(ii))
        return RAGE_FAILURE_CAST(rage_Error, ii);
    rage_Interpolator * interpolator = RAGE_SUCCESS_VALUE(ii);
    rage_Error err = checker(interpolator);
    rage_interpolator_free(&td, interpolator);
    return err;
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

static rage_Error float_checks(rage_Interpolator * interpolator) {
    rage_InterpolatedView * v = rage_interpolator_get_view(interpolator, 0);
    if (f_check(v, 0.0))
        return RAGE_ERROR("Mismatch at t=0");
    rage_interpolated_view_advance(v, 1);
    if (f_check(v, 0.5))
        return RAGE_ERROR("Mismatch at t=1");
    rage_interpolated_view_advance(v, 1);
    if (f_check(v, 1.0))
        return RAGE_ERROR("Mismatch at t=2");
    rage_interpolated_view_advance(v, 1);
    if (f_check(v, 1.0))
        return RAGE_ERROR("Mismatch at t=3");
    rage_interpolated_view_advance(v, 1);
    if (f_check(v, 2.0))
        return RAGE_ERROR("Mismatch at t=4");
    rage_interpolated_view_advance(v, 1);
    if (f_check(v, 2.0))
        return RAGE_ERROR("Mismatch at t=5");
    return RAGE_OK;
}

static rage_AtomDef const unconstrained_float = {
    .type = RAGE_ATOM_FLOAT, .name = "float", .constraints = {}};

static rage_Error interpolator_float_test() {
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
        &unconstrained_float, tps, 3, float_checks, 1);
}

RAGE_EQUALITY_CHECK(int, i, "%i")

static rage_Error int_checks(rage_Interpolator * interpolator) {
    rage_InterpolatedView * v = rage_interpolator_get_view(interpolator, 0);
    if (rage_interpolated_view_get_pos(v) != 0)
        return RAGE_ERROR("Not starting at the start");
    if (i_check(v, 10))
        return RAGE_ERROR("Incorrect initial value");
    rage_interpolated_view_advance(v, 5);
    if (i_check(v, 15))
        return RAGE_ERROR("Incorrect halfway value");
    rage_interpolated_view_advance(v, 100);
    if (i_check(v, 20))
        return RAGE_ERROR("Incorrect final value");
    return RAGE_OK;
}

static rage_AtomDef const unconstrained_int = {
    .type = RAGE_ATOM_INT, .name = "int", .constraints = {}};

static rage_Error interpolator_int_test() {
    rage_Atom vals[] = {{.i = 10}, {.i = 20}};
    rage_TimePoint tps[] = {
        {
            .time = {.second = 0},
            .value = &(vals[0]),
            .mode = RAGE_INTERPOLATION_LINEAR
        },
        {
            .time = {.second = 10},
            .value = &(vals[1]),
            .mode = RAGE_INTERPOLATION_CONST
        }
    };
    return check_with_single_field_interpolator(
        &unconstrained_int, tps, 2, int_checks, 1);
}

RAGE_EQUALITY_CHECK(uint32_t, frame_no, "%u")

static rage_Error time_checks(rage_Interpolator * interpolator) {
    rage_InterpolatedView * v = rage_interpolator_get_view(interpolator, 0);
    if (frame_no_check(v, 0))
        return RAGE_ERROR("Mismatch at t=0");
    rage_interpolated_view_advance(v, 1);
    if (frame_no_check(v, 1))
        return RAGE_ERROR("Mismatch at t=1");
    rage_interpolated_view_advance(v, 2);
    if (frame_no_check(v, 3))
        return RAGE_ERROR("Mismatch at t=3");
    rage_interpolated_view_advance(v, 1);
    if (frame_no_check(v, 1))
        return RAGE_ERROR("Mismatch at t=4");
    rage_interpolated_view_advance(v, 120);
    if (frame_no_check(v, 121))
        return RAGE_ERROR("Mismatch at t=124");
    return RAGE_OK;
}

static rage_Error interpolator_time_test() {
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
        &unconstrained_time, tps, 2, time_checks, 1);
}

static rage_Error interpolator_new_with_no_timepoints() {
    rage_Error err = check_with_single_field_interpolator(
        &unconstrained_int, NULL, 0, NULL, 1);
    if (!RAGE_FAILED(err))
        return RAGE_ERROR("Interpolator create did not fail");
    return RAGE_OK;
}

static rage_Error interpolator_new_first_timepoint_not_start() {
    rage_Atom vals[] = {
        {.i = 0}
    };
    rage_TimePoint tps[] = {
        {
            .time = {.second = 20},
            .value = &(vals[0]),
            .mode = RAGE_INTERPOLATION_CONST
        }
    };
    rage_Error err = check_with_single_field_interpolator(
        &unconstrained_int, tps, 1, NULL, 1);
    if (!RAGE_FAILED(err))
        return RAGE_ERROR("Interpolator create did not fail");
    return RAGE_OK;
}

static rage_Error interpolator_ambiguous_interpolation_mode() {
    rage_Atom vals[] = {
        {.i = 0}
    };
    rage_TimePoint tps[] = {
        {
            .time = {.second = 0},
            .value = &(vals[0]),
            .mode = 300
        },
        {
            .time = {.second = 1},
            .value = &(vals[0]),
            .mode = RAGE_INTERPOLATION_CONST
        }
    };
    rage_Error err = check_with_single_field_interpolator(
        &unconstrained_int, tps, 2, NULL, 1);
    if (!RAGE_FAILED(err))
        return RAGE_ERROR("Interpolator create did not fail");
    return RAGE_OK;
}

static rage_Error interpolator_timeseries_not_monotonic() {
    rage_Atom vals[] = {
        {.i = 0}
    };
    rage_TimePoint tps[] = {
        {
            .time = {.second = 0},
            .value = &(vals[0]),
            .mode = RAGE_INTERPOLATION_CONST
        },
        {
            .time = {.second = 3},
            .value = &(vals[0]),
            .mode = RAGE_INTERPOLATION_CONST
        },
        {
            .time = {.second = 2},
            .value = &(vals[0]),
            .mode = RAGE_INTERPOLATION_CONST
        }
    };
    rage_Error err = check_with_single_field_interpolator(
        &unconstrained_int, tps, 3, NULL, 1);
    if (!RAGE_FAILED(err))
        return RAGE_ERROR("Interpolator create did not fail");
    return RAGE_OK;
}

static rage_Error check_float_value(
        rage_InterpolatedValue const * val, float expected,
        uint32_t duration) {
    if (val->value[0].f != expected)
        return RAGE_ERROR("Incorrect interpolated value");
    if (val->valid_for != duration)
        return RAGE_ERROR("Incorrect validity duration");
    return RAGE_OK;
}

static rage_Finaliser * change_float_ts_to(
        rage_Interpolator * interpolator, float f, uint32_t frame) {
    rage_Atom val = {.f = f};
    rage_TimePoint tps[] = {
        {
            .time = {.second = 0},
            .value = &val,
            .mode = RAGE_INTERPOLATION_CONST
        },
    };
    rage_TimeSeries ts = {
        .len = 1,
        .items = tps
    };
    return rage_interpolator_change_timeseries(interpolator, &ts, frame);
}

static rage_Error immediate_change_checks(rage_Interpolator * interpolator) {
    rage_InterpolatedView * iv = rage_interpolator_get_view(interpolator, 0);
    rage_InterpolatedValue const * obtained = rage_interpolated_view_value(iv);
    rage_Error rv = check_float_value(obtained, 0.0, UINT32_MAX);
    if (!RAGE_FAILED(rv)) {
        rage_Finaliser * change_complete = change_float_ts_to(interpolator, 1.0, 0);
        // FIXME: should reading the value trigger a seek?
        rage_interpolated_view_advance(iv, 0);
        obtained = rage_interpolated_view_value(iv);
        rage_finaliser_wait(change_complete);
        rv = check_float_value(obtained, 1.0, UINT32_MAX);
    }
    return rv;
}

static rage_Error zero_float_start(
        rage_Error (*checker)(rage_Interpolator *), uint8_t n_views) {
    rage_Atom val = {.f = 0};
    rage_TimePoint tps[] = {
        {
            .time = {.second = 0},
            .value = &val,
            .mode = RAGE_INTERPOLATION_CONST
        },
    };
    return check_with_single_field_interpolator(
        &unconstrained_float, tps, 1, checker, n_views);
}

static rage_Error interpolator_immediate_change_test() {
    return zero_float_start(immediate_change_checks, 1);
}

static rage_Error delayed_change_checks(rage_Interpolator * interpolator) {
    rage_Error rv = RAGE_OK;
    for (uint8_t i = 0; i < 2; i++) {
        rage_InterpolatedView * v = rage_interpolator_get_view(interpolator, i);
        rage_InterpolatedValue const * r = rage_interpolated_view_value(v);
        rv = check_float_value(r, 0.0, UINT32_MAX);
        if (RAGE_FAILED(rv))
            return rv;
    }
    rage_Finaliser * change_complete = change_float_ts_to(interpolator, 1.0, 2);
    for (uint8_t i = 0; i < 2; i++) {
        rage_InterpolatedView * v = rage_interpolator_get_view(interpolator, i);
        rage_interpolated_view_advance(v, 1);
        rage_InterpolatedValue const * r = rage_interpolated_view_value(v);
        rv = check_float_value(r, 0.0, 1);
        if (RAGE_FAILED(rv))
            break;
        rage_interpolated_view_advance(v, 1);
        r = rage_interpolated_view_value(v);
        rv = check_float_value(r, 1.0, UINT32_MAX);
        if (RAGE_FAILED(rv))
            break;
    }
    rage_finaliser_wait(change_complete);
    return rv;
}

static rage_Error interpolator_delayed_change_test() {
    return zero_float_start(delayed_change_checks, 2);
}

// TODO: change part way through to halfway through interpolation test
