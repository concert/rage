#pragma once

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
