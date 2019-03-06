#pragma once
#include "loader.h"

rage_Atom ** generate_tuples(rage_ParamDefList const * const pdl);
void free_tuples(rage_ParamDefList const * const pdl, rage_Atom ** as);

typedef struct {
    rage_ElementLoader * loader;
    rage_LoadedElementKind * kind;
    rage_ConcreteElementType * cet;
    rage_Element * elem;
    uint32_t elem_frame_size;
} rage_TestElem;

typedef RAGE_OR_ERROR(rage_TestElem) rage_NewTestElem;

void free_test_elem(rage_TestElem te);
rage_NewTestElem new_test_elem(char const * elem_so);
