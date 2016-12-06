#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "atoms.h"

rage_Atom * rage_tuple_generate(rage_TupleDef const * const td) {
    rage_Atom * tup = calloc(td->len, sizeof(rage_Atom));
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

rage_Atom * rage_tuple_copy(rage_TupleDef const * const td, rage_Atom const * const orig) {
    rage_Atom * copy = calloc(td->len, sizeof(rage_Atom));
    for (unsigned i=0; i < td->len; i++) {
        rage_AtomDef const * const at = td->items[i].type;
        switch (at->type) {
            case RAGE_ATOM_INT:
            case RAGE_ATOM_FLOAT:
            case RAGE_ATOM_TIME:
                copy[i] = orig[i];
                break;
            case RAGE_ATOM_STRING:
                copy[i].s = malloc(strlen(orig[i].s));
                strcpy(copy[i].s, orig[i].s);
                break;
        }
    }
    return copy;
}

void rage_tuple_free(rage_TupleDef const * const td, rage_Atom * const tup) {
    for (unsigned i=0; i < td->len; i++) {
        rage_AtomDef const * const at = td->items[i].type;
        switch (at->type) {
            case RAGE_ATOM_STRING:
                free(tup[i].s);
                break;
            case RAGE_ATOM_INT:
            case RAGE_ATOM_FLOAT:
            case RAGE_ATOM_TIME:
                break;
        }
    }
    free(tup);
}
