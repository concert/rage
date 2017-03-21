#include "countdown.h"
#include "srt.h"

typedef struct {
    rage_ElementLoader * loader;
    rage_ElementType * type;
    rage_Element * elem;
} rage_TestElem;

typedef RAGE_OR_ERROR(rage_TestElem) rage_NewTestElem;

rage_NewTestElem new_test_elem() {
    rage_ElementLoader * el = rage_element_loader_new();
    rage_ElementTypeLoadResult et_ = rage_element_loader_load(
        el, "./libamp.so");
    if (RAGE_FAILED(et_)) {
        rage_element_loader_free(el);
        RAGE_FAIL(rage_NewTestElem, RAGE_FAILURE_VALUE(et_));
    }
    rage_ElementType * et = RAGE_SUCCESS_VALUE(et_);
    rage_Atom * tup = rage_tuple_generate(et->parameters);
    rage_ElementNewResult elem_ = rage_element_new(et, 44100, 256, tup);
    free(tup);
    if (RAGE_FAILED(elem_)) {
        rage_element_loader_unload(el, et);
        rage_element_loader_free(el);
        RAGE_FAIL(rage_NewTestElem, RAGE_FAILURE_VALUE(elem_));
    }
    rage_TestElem rv = {
        .loader=el, .type=et, .elem=RAGE_SUCCESS_VALUE(elem_)};
    RAGE_SUCCEED(rage_NewTestElem, rv);
}

void free_test_elem(rage_TestElem te) {
    rage_element_free(te.elem);
    rage_element_loader_unload(te.loader, te.type);
    rage_element_loader_free(te.loader);
}

rage_Error test_srt() {
    rage_NewTestElem nte = new_test_elem();
    RAGE_EXTRACT_VALUE(rage_Error, nte, rage_TestElem te);
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
