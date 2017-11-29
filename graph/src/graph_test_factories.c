#include "graph_test_factories.h"
#include "test_factories.h"

rage_Atom ** generate_tuples(rage_ParamDefList const * const pdl) {
    rage_Atom ** as = calloc(sizeof(rage_Atom *), pdl->len);
    for (uint32_t i = 0; i < pdl->len; i++) {
        as[i] = rage_tuple_generate(pdl->items + i);
    }
    return as;
}

void free_tuples(rage_ParamDefList const * const pdl, rage_Atom ** as) {
    for (uint32_t i = 0; i < pdl->len; i++) {
        free(as[i]);
    }
    free(as);
}

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
