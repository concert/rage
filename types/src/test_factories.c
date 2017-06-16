#include <stdlib.h>
#include <assert.h>
#include "atoms.h"
#include "time_series.h"

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

rage_TimeSeries rage_time_series_new(rage_TupleDef const * const item_def) {
    rage_TimePoint * tp = malloc(sizeof(rage_TimePoint));
    tp->time = (rage_Time) {.second = 0};
    tp->value = rage_tuple_generate(item_def);
    tp->mode = RAGE_INTERPOLATION_CONST;
    return (rage_TimeSeries) {.len = 1, .items = tp};
}

void rage_time_series_free(rage_TimeSeries ts) {
    for (uint32_t i = 0; i < ts.len; i++) {
        free(ts.items[i].value);
    }
    free(ts.items);
}
