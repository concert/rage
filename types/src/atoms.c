#include <stdlib.h>
#include <assert.h>
#include "atoms.h"

rage_Tuple rage_tuple_generate(rage_TupleDef const * const td) {
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
