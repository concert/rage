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

static rage_PreparedFrames fake_elem_prep(
        rage_ElementState * state, rage_InterpolatedView ** controls) {
    state->prep_ctls = controls;
    state->prep_counter++;
    sem_post(state->processed);
    return RAGE_SUCCESS(rage_PreparedFrames, 1024);
}

static rage_PreparedFrames fake_elem_clean(
        rage_ElementState * state, rage_InterpolatedView ** controls) {
    state->clean_ctls = controls;
    state->clean_counter++;
    return RAGE_SUCCESS(rage_PreparedFrames, 1024);
}

static rage_ElementType fake_element_type = {
    .prep = fake_elem_prep,
    .clean = fake_elem_clean
};

static rage_ConcreteElementType fake_concrete_type = {
    .type = &fake_element_type
};

static rage_Error counter_checks(
        sem_t * sync_sem, rage_ElementState * fes, int expected_prep,
        int expected_clean) {
    sem_wait(sync_sem);
    if (fes->prep_counter != expected_prep) {
        return RAGE_ERROR("Unexpected prep count");
    }
    if (fes->clean_counter != expected_clean) {
        return RAGE_ERROR("Unexpected clean count");
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
    rage_InterpolatedView **prep_view = (void *) 134, **clean_view = (void *) 983;
    rage_Element fake_elem = {
        .cet = &fake_concrete_type,
        .state = &fes};
    rage_Countdown * countdown = rage_countdown_new(0);
    rage_SupportConvoy * convoy = rage_support_convoy_new(1024, countdown);
    rage_Error err = rage_support_convoy_start(convoy);
    rage_SupportTruck * truck = rage_support_convoy_mount(
        convoy, &fake_elem, prep_view, clean_view);
    // FIXME: ATM don't know (without our sem) when initial prep is done (which
    // is dodgy)
    rage_Error assertion_err = RAGE_OK;
    if (!RAGE_FAILED(err)) {
        assertion_err = counter_checks(&sync_sem, &fes, 1, 0);
        if (!RAGE_FAILED(assertion_err)) {
            rage_countdown_add(countdown, -1);
            assertion_err = counter_checks(&sync_sem, &fes, 2, 1);
        }
        err = rage_support_convoy_stop(convoy);
    }
    rage_support_convoy_unmount(truck);
    rage_support_convoy_free(convoy);
    rage_countdown_free(countdown);
    sem_destroy(&sync_sem);
    if (RAGE_FAILED(err)) {
        return err;
    }
    if (RAGE_FAILED(assertion_err)) {
        return assertion_err;
    }
    if (fes.prep_ctls != prep_view) {
        return RAGE_ERROR("Bad prep ctl");
    }
    if (fes.clean_ctls != clean_view) {
        return RAGE_ERROR("Bad clean ctl");
    }
    return RAGE_OK;
}
