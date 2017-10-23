#include <semaphore.h>
#include "countdown.h"
#include "srt.h"
#include "pdls.c"
#include "loader.h"

typedef struct {
    rage_ElementLoader * loader;
    rage_ElementKind * kind;
    rage_ConcreteElementType * cet;
    rage_Element * elem;
} rage_TestElem;

typedef RAGE_OR_ERROR(rage_TestElem) rage_NewTestElem;

void free_test_elem(rage_TestElem te) {
    if (te.elem != NULL)
        rage_element_free(te.elem);
    if (te.cet != NULL)
        rage_concrete_element_type_free(te.cet);
    if (te.kind != NULL)
        rage_element_loader_unload(te.kind);
    if (te.loader != NULL)
        rage_element_loader_free(te.loader);
}

rage_NewTestElem new_test_elem() {
    rage_ElementLoader * el = rage_element_loader_new(getenv("RAGE_ELEMENTS_PATH"));
    rage_TestElem rv = {.loader=el};
    rage_ElementKindLoadResult ek_ = rage_element_loader_load("./libamp.so");
    if (RAGE_FAILED(ek_)) {
        free_test_elem(rv);
        return RAGE_FAILURE_CAST(rage_NewTestElem, ek_);
    }
    rv.kind = RAGE_SUCCESS_VALUE(ek_);
    rage_Atom ** tups = generate_tuples(rage_element_kind_parameters(rv.kind));
    rage_NewConcreteElementType ncet = rage_element_type_specialise(
        rv.kind, tups);
    free_tuples(rage_element_kind_parameters(rv.kind), tups);
    if (RAGE_FAILED(ncet)) {
        free_test_elem(rv);
        return RAGE_FAILURE_CAST(rage_NewTestElem, ncet);
    }
    rv.cet = RAGE_SUCCESS_VALUE(ncet);
    rage_ElementNewResult elem_ = rage_element_new(rv.cet, 44100, 256);
    if (RAGE_FAILED(elem_)) {
        free_test_elem(rv);
        return RAGE_FAILURE_CAST(rage_NewTestElem, elem_);
    }
    rv.elem = RAGE_SUCCESS_VALUE(elem_);
    return RAGE_SUCCESS(rage_NewTestElem, rv);
}

rage_Error test_srt() {
    rage_NewTestElem nte = new_test_elem();
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
};

static rage_PreparedFrames fake_elem_prep(
        rage_ElementState * state, rage_InterpolatedView ** controls) {
    state->prep_counter++;
    sem_post(state->processed);
    return RAGE_SUCCESS(rage_PreparedFrames, 1024);
}

static rage_PreparedFrames fake_elem_clean(
        rage_ElementState * state, rage_InterpolatedView ** controls) {
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

rage_Error test_srt_fake_elem() {
    sem_t sync_sem;
    sem_init(&sync_sem, 0, 0);
    rage_ElementState fes = {
        .prep_counter = 0,
        .clean_counter = 0,
        .processed = &sync_sem};
    rage_Element fake_elem = {
        .cet = &fake_concrete_type,
        .state = &fes};
    rage_Countdown * countdown = rage_countdown_new(0);
    rage_SupportConvoy * convoy = rage_support_convoy_new(1024, countdown);
    rage_SupportTruck * truck = rage_support_convoy_mount(
        convoy, &fake_elem, (rage_InterpolatedView **) 1,
        (rage_InterpolatedView **) 1);
    // FIXME: ATM don't know (without our sem) when initial prep is done (which
    // is dodgy)
    rage_Error err = rage_support_convoy_start(convoy);
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
    } else {
        return assertion_err;
    }
}
