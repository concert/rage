#include <semaphore.h>
#include "countdown.h"
#include "srt.h"
#include "graph_test_factories.h"

rage_Error test_srt() {
    rage_NewTestElem nte = new_test_elem("./libamp.so");
    if (RAGE_FAILED(nte))
        return RAGE_FAILURE_CAST(rage_Error, nte);
    rage_TestElem te = RAGE_SUCCESS_VALUE(nte);
    rage_SupportConvoy * convoy = rage_support_convoy_new(
        RAGE_TRANSPORT_STOPPED, NULL);
    rage_SupportTruck * truck = rage_support_convoy_mount(
        convoy, te.elem, NULL, NULL);
    rage_Error err = rage_support_convoy_start(convoy);
    if (!RAGE_FAILED(err)) {
        err = rage_support_convoy_stop(convoy);
    }
    rage_support_convoy_unmount(truck);
    rage_support_convoy_free(convoy);
    free_test_elem(te);
    return err;
}

struct rage_ElementState {
    int prep_counter;
    int last_prep_from;
    int clean_counter;
    int last_clean_from;
    int clear_counter;
    int last_clear_from;
    sem_t * processed;
    rage_InterpolatedView ** prep_ctls;
    rage_InterpolatedView ** clean_ctls;
};

static rage_Error fake_elem_prep(
        rage_ElementState * state, rage_InterpolatedView ** controls) {
    state->prep_ctls = controls;
    state->prep_counter++;
    state->last_prep_from = rage_interpolated_view_get_pos(controls[0]);
    sem_post(state->processed);
    rage_interpolated_view_advance(controls[0], 2048);
    return RAGE_OK;
}

static rage_Error fake_elem_clean(
        rage_ElementState * state, rage_InterpolatedView ** controls) {
    state->clean_ctls = controls;
    state->clean_counter++;
    state->last_clean_from = rage_interpolated_view_get_pos(controls[0]);
    rage_interpolated_view_advance(controls[0], 2048);
    return RAGE_OK;
}

static rage_Error fake_elem_clear(
        rage_ElementState * state, rage_InterpolatedView ** controls,
        rage_FrameNo to_present) {
    state->clear_counter++;
    state->last_clear_from = rage_interpolated_view_get_pos(controls[0]);
    rage_interpolated_view_advance(controls[0], 420);
    return RAGE_OK;
}

static rage_TupleDef const empty_tupledef = {};

static rage_ElementType fake_type = {
    .spec = (rage_InstanceSpec) {.controls = {
        .len = 1, .items = &empty_tupledef}},
    .prep = fake_elem_prep,
    .clean = fake_elem_clean,
    .clear = fake_elem_clear
};

#define ERR_MSG_BUFF_SIZE 128
static char err_msg[ERR_MSG_BUFF_SIZE];

static rage_Error counter_checks(
        sem_t * sync_sem, rage_ElementState * fes, int expected_prep,
        int expected_clean) {
    sem_wait(sync_sem);
    if (fes->prep_counter != expected_prep) {
        snprintf(
            err_msg, ERR_MSG_BUFF_SIZE, "Unexpected prep count %u != %u",
            fes->prep_counter, expected_prep);
        return RAGE_ERROR(err_msg);
    }
    if (fes->clean_counter != expected_clean) {
        snprintf(
            err_msg, ERR_MSG_BUFF_SIZE, "Unexpected clean count %u != %u",
            fes->clean_counter, expected_clean);
        return RAGE_ERROR(err_msg);
    }
    return RAGE_OK;
}

static rage_Error test_srt_fake_elem() {
    sem_t sync_sem;
    sem_init(&sync_sem, 0, 0);
    rage_ElementState fes = {
        .last_prep_from = -1,
        .last_clean_from =  -1,
        .last_clear_from = -1,
        .processed = &sync_sem};
    rage_Error assertion_err = RAGE_OK;
    rage_TimePoint tp = {};
    rage_TimeSeries ts = {.len = 1, .items = &tp};
    rage_InterpolatedView *prep_view, *clean_view;
    rage_Element fake_elem = {
        .type = &fake_type,
        .state = &fes};
    rage_SupportConvoy * convoy = rage_support_convoy_new(
        RAGE_TRANSPORT_STOPPED, NULL);
    rage_Countdown * countdown = rage_support_convoy_get_countdown(convoy);
    rage_Error err = rage_support_convoy_start(convoy);
    if (!RAGE_FAILED(err)) {
        rage_InitialisedInterpolator ii = rage_interpolator_new(
            &empty_tupledef, &ts, 44100, 2, NULL);
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
                rage_countdown_add(countdown, -1024);
                rage_countdown_add(countdown, -1024);
                assertion_err = counter_checks(&sync_sem, &fes, 2, 2);
                if (!RAGE_FAILED(assertion_err)) {
                    if (fes.last_prep_from != 2048 || fes.last_clean_from != 2048) {
                        assertion_err = RAGE_ERROR("Bad prep start point");
                    } else {
                        assertion_err = rage_support_convoy_transport_seek(convoy, 12);
                        if (!RAGE_FAILED(assertion_err)) {
                            assertion_err = counter_checks(&sync_sem, &fes, 3, 3);
                            if (!RAGE_FAILED(assertion_err)) {
                                if (fes.clear_counter != 1) {
                                    assertion_err = RAGE_ERROR("Did not clear on seek");
                                } else if (fes.last_clear_from != 2048) {
                                    assertion_err = RAGE_ERROR("Clear called in wrong place");
                                } else if (fes.last_prep_from != 12) {
                                    assertion_err = RAGE_ERROR("Prep after seek in wrong place");
                                } else if (fes.last_clean_from != 12) {
                                    assertion_err = RAGE_ERROR("Clean after seek in wrong place");
                                }
                            }
                        }
                    }
                }
            }
            rage_support_convoy_unmount(truck);
            rage_interpolator_free(&empty_tupledef, interp);
        }
    }
    err = rage_support_convoy_stop(convoy);
    rage_support_convoy_free(convoy);
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
