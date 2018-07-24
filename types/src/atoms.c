#include <stdlib.h>
#include <string.h>
#include "atoms.h"

rage_Atom * rage_tuple_copy(rage_TupleDef const * const td, rage_Atom const * const orig) {
    rage_Atom * copy = calloc(td->len, sizeof(rage_Atom));
    for (unsigned i=0; i < td->len; i++) {
        rage_AtomDef const * const at = td->items[i].type;
        switch (at->type) {
            case RAGE_ATOM_INT:
            case RAGE_ATOM_FLOAT:
            case RAGE_ATOM_TIME:
            case RAGE_ATOM_ENUM:
                copy[i] = orig[i];
                break;
            case RAGE_ATOM_STRING:
                copy[i].s = strdup(orig[i].s);
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
            case RAGE_ATOM_ENUM:
                break;
        }
    }
    free(tup);
}
