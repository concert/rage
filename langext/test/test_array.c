#include <stdlib.h>
#include "error.h"
#include "macros.h"

typedef RAGE_ARRAY(int) IntArray;

rage_Error test_array_init() {
    IntArray arr;
    RAGE_ARRAY_INIT(&arr, 2, i) {
        arr.items[i] = i;
    }
    rage_Error err = RAGE_OK;
    if (arr.len != 2) {
        err = RAGE_ERROR("Wrong length");
    }
    free(arr.items);
    return err;
}

typedef RAGE_ARRAY(char const *) StringArray;

static RAGE_POINTER_ARRAY_APPEND_FUNC_DEF(StringArray, char const, append_string)

rage_Error test_array_append() {
    StringArray arr = {.len=0, .items=NULL};
    char const * hi = "Hello";
    StringArray * ext = append_string(&arr, hi);
    rage_Error err = RAGE_OK;
    if (arr.len != 0 || arr.items != NULL) {
        err = RAGE_ERROR("Original array mutated");
    } else {
        if (ext->len != 1 || ext->items[0] != hi) {
            err = RAGE_ERROR("Issues with extending from empty");
        }
    }
    free(ext->items);
    free(ext);
    return err;
}
