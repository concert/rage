#include <stdio.h>
#include <stdlib.h>
#include "loader.h"
#include <assert.h>

rage_Tuple generate_valid_tuple(rage_TupleDef const * td) {
    rage_Tuple tup = calloc(td->len, sizeof(rage_Atom));
    for (unsigned i=0; i < td->len; i++) {
        rage_AtomDef const * const at = td->items[i].type;
        switch (at->type) {
            case RAGE_ATOM_INT: {
                int32_t v = 0;
                if (at->constraints.i.min.half == RAGE_EITHER_RIGHT)
                    v = at->constraints.i.min.right;
                if (at->constraints.i.max.half == RAGE_EITHER_RIGHT)
                    v = at->constraints.i.max.right;
                tup[i].i = v;
                break;
            }
            case RAGE_ATOM_FLOAT: {
                float v = 0;
                if (at->constraints.f.min.half == RAGE_EITHER_RIGHT)
                    v = at->constraints.f.min.right;
                if (at->constraints.f.max.half == RAGE_EITHER_RIGHT)
                    v = at->constraints.f.max.right;
                tup[i].f = v;
                break;
            }
            case RAGE_ATOM_TIME:
            case RAGE_ATOM_STRING:
                assert(false);
        }
    }
    return tup;
}

rage_Error test() {
    rage_ElementLoader el = rage_element_loader_new();
    rage_ElementTypes element_type_names = rage_element_loader_list(el);
    for (unsigned i=0; i<element_type_names.len; i++) {
        rage_ElementTypeLoadResult et_ = rage_element_loader_load(
            el, element_type_names.items[i]);
        RAGE_EXTRACT_VALUE(rage_Error, et_, rage_ElementType * et)
        rage_Tuple tup = generate_valid_tuple(et->parameters);
        rage_ElementNewResult elem_ = rage_element_new(et, 44100, 256, tup);
        RAGE_EXTRACT_VALUE(rage_Error, elem_, rage_Element * elem)
        rage_element_free(elem);
        free(tup);
        rage_element_loader_unload(el, et);
    }
    rage_element_loader_free(el);
    RAGE_OK
}

int main() {
    rage_Error e = test();
    if (RAGE_FAILED(e)) {
        printf("Failed: %s\n", RAGE_FAILURE_VALUE(e));
        return 1;
    }
}
