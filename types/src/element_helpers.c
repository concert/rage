#include "element_helpers.h"

uint8_t view_count_for_type(rage_ElementType const * const type) {
    uint8_t n_views = 1;
    if (type->prep != NULL) {
        n_views++;
    }
    if (type->clean != NULL) {
        n_views++;
    }
    return n_views;
}

