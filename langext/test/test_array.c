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
