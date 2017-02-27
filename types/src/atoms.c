#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "atoms.h"

rage_Atom * rage_tuple_generate(rage_TupleDef const * const td) {
    rage_Atom * tup = calloc(td->len, sizeof(rage_Atom));
    for (unsigned i=0; i < td->len; i++) {
        rage_AtomDef const * const at = td->items[i].type;
        #define RAGE_MINMAX_GENERATOR(type, member, initial_value) { \
            type v = initial_value; \
            if (at->constraints.member.min.half == RAGE_EITHER_RIGHT) \
                v = at->constraints.member.min.right; \
            if (at->constraints.member.max.half == RAGE_EITHER_RIGHT) \
                v = at->constraints.member.max.right; \
            tup[i].member = v; \
            break;}
        switch (at->type) {
            case RAGE_ATOM_INT:
                RAGE_MINMAX_GENERATOR(int32_t, i, 0)
            case RAGE_ATOM_FLOAT:
                RAGE_MINMAX_GENERATOR(float, f, 0.0)
            case RAGE_ATOM_TIME:
                RAGE_MINMAX_GENERATOR(rage_Time, t, {})
            case RAGE_ATOM_ENUM:
                assert(at->constraints.e.len);
                tup[i].e = at->constraints.e.items[0].value;
                break;
            case RAGE_ATOM_STRING:
                assert(at->constraints.s.half == RAGE_EITHER_LEFT);
                tup[i].s = "";
        }
        #undef RAGE_MINMAX_GENERATOR
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
            case RAGE_ATOM_ENUM:
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
            case RAGE_ATOM_ENUM:
                break;
        }
    }
    free(tup);
}
