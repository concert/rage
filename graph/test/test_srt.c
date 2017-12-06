#include <semaphore.h>
#include "countdown.h"
#include "srt.h"
#include "graph_test_factories.h"

rage_Error test_srt() {
    rage_NewTestElem nte = new_test_elem("./libamp.so");
    if (RAGE_FAILED(nte))
        return RAGE_FAILURE_CAST(rage_Error, nte);
    rage_TestElem te = RAGE_SUCCESS_VALUE(nte);
    rage_Countdown * countdown = rage_countdown_new(0);
    rage_SupportConvoy * convoy = rage_support_convoy_new(1024, countdown);
    rage_SupportTruck * truck = rage_support_convoy_mount(convoy, te.elem, NULL, NULL);
    rage_Error err = rage_support_convoy_start(convoy);
    if (!RAGE_FAILED(err)) {
        err = rage_support_convoy_stop(convoy);
    }
    rage_support_convoy_unmount(truck);
    rage_support_convoy_free(convoy);
    rage_countdown_free(countdown);
    free_test_elem(te);
    return err;
}

struct rage_ElementState {
    int prep_counter;
    int clean_counter;
    sem_t * processed;
    rage_InterpolatedView ** prep_ctls;
    rage_InterpolatedView ** clean_ctls;
};

static rage_Error fake_elem_prep(
        rage_ElementState * state, rage_InterpolatedView ** controls) {
    state->prep_ctls = controls;
    state->prep_counter++;
    sem_post(state->processed);
    rage_interpolated_view_advance(controls[0], 2048);
    return RAGE_OK;
}

static rage_Error fake_elem_clean(
        rage_ElementState * state, rage_InterpolatedView ** controls) {
    state->clean_ctls = controls;
    state->clean_counter++;
    rage_interpolated_view_advance(controls[0], 2048);
    return RAGE_OK;
}

static rage_ElementType fake_element_type = {
    .prep = fake_elem_prep,
    .clean = fake_elem_clean
};

static rage_ConcreteElementType fake_concrete_type = {
    .type = &fake_element_type
};

#define ERR_MSG_BUFF_SIZE 128
static char err_msg[ERR_MSG_BUFF_SIZE];

static rage_Error counter_checks(
        sem_t * sync_sem, rage_ElementState * fes, int expected_prep,
        int expected_clean) {
    sem_wait(sync_sem);
    if (fes->prep_counter != expected_prep) {
        snprintf(err_msg, ERR_MSG_BUFF_SIZE, "Unexpected prep count %u != %u", fes->prep_counter, expected_prep);
        return RAGE_ERROR(err_msg);
    }
    if (fes->clean_counter != expected_clean) {
        snprintf(err_msg, ERR_MSG_BUFF_SIZE, "Unexpected clean count %u != %u", fes->clean_counter, expected_clean);
        return RAGE_ERROR(err_msg);
    }
    return RAGE_OK;
}

static rage_Error test_srt_fake_elem() {
    sem_t sync_sem;
    sem_init(&sync_sem, 0, 0);
    rage_ElementState fes = {
        .prep_counter = 0,
        .clean_counter = 0,
        .processed = &sync_sem};
    rage_Error assertion_err = RAGE_OK;
    rage_TimePoint tp = {};
    rage_TimeSeries ts = {.len = 1, .items = &tp};
    rage_TupleDef td = {};
    rage_InterpolatedView *prep_view, *clean_view;
    rage_Element fake_elem = {
        .cet = &fake_concrete_type,
        .state = &fes};
    rage_Countdown * countdown = rage_countdown_new(0);
    rage_SupportConvoy * convoy = rage_support_convoy_new(1024, countdown);
    rage_Error err = rage_support_convoy_start(convoy);
    if (!RAGE_FAILED(err)) {
        rage_InitialisedInterpolator ii = rage_interpolator_new(&td, &ts, 44100, 2);
        if (RAGE_FAILED(ii)) {
            assertion_err = RAGE_FAILURE_CAST(rage_Error, ii);
        } else {
            rage_Interpolator * interp = RAGE_SUCCESS_VALUE(ii);
            prep_view = rage_interpolator_get_view(interp, 0);
            clean_view = rage_interpolator_get_view(interp, 1);
            rage_SupportTruck * truck = rage_support_convoy_mount(
                convoy, &fake_elem, &prep_view, &clean_view);
            assertion_err = counter_checks(&sync_sem, &fes, 1, 1);
            if (!RAGE_FAILED(assertion_err)) {
                rage_countdown_add(countdown, -1);
                rage_countdown_add(countdown, -1);
                assertion_err = counter_checks(&sync_sem, &fes, 2, 2);
            }
            rage_support_convoy_unmount(truck);
            rage_interpolator_free(&td, interp);
        }
    }
    err = rage_support_convoy_stop(convoy);
    rage_support_convoy_free(convoy);
    rage_countdown_free(countdown);
    sem_destroy(&sync_sem);
    if (RAGE_FAILED(err)) {
        return err;
    }
    if (RAGE_FAILED(assertion_err)) {
        return assertion_err;
    }
    if (fes.prep_ctls != &prep_view) {
        return RAGE_ERROR("Bad prep ctl");
    }
    if (fes.clean_ctls != &clean_view) {
        return RAGE_ERROR("Bad clean ctl");
    }
    return RAGE_OK;
}
