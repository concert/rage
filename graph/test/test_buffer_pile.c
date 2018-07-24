#include "buffer_pile.h"
#include "error.h"

void fill_buffers(rage_BuffersInfo const * b) {
    for (unsigned i = 0; i < b->n_buffers; i++) {
        for (unsigned j = 0; j < b->buffer_size; j++) {
            b->buffers[i][j] = i + j;
        }
    }
}

rage_Error test_buffer_pile() {
    rage_Error err = RAGE_OK;
    rage_BufferAllocs * ba0 = rage_buffer_allocs_new(3);
    rage_BufferAllocs * ba1 = rage_buffer_allocs_alloc(ba0, 1);
    fill_buffers(rage_buffer_allocs_get_buffers_info(ba1));
    rage_buffer_allocs_free(ba0);
    ba0 = rage_buffer_allocs_alloc(ba1, 2);
    fill_buffers(rage_buffer_allocs_get_buffers_info(ba0));
    rage_buffer_allocs_free(ba1);
    ba1 = rage_buffer_allocs_alloc(ba0, 1);
    fill_buffers(rage_buffer_allocs_get_buffers_info(ba1));
    rage_buffer_allocs_free(ba1);
    rage_buffer_allocs_free(ba0);
    return err;
}
