#pragma once
#include <stddef.h>

typedef struct {
    size_t buffer_size;
    unsigned n_buffers;
    char ** buffers;
} rage_BuffersInfo;

typedef struct rage_BufferAllocs rage_BufferAllocs;

rage_BufferAllocs * rage_buffer_allocs_new(size_t const buffer_size);
rage_BufferAllocs * rage_buffer_allocs_alloc(
    rage_BufferAllocs * const current_alloc, unsigned const n_buffers);
void rage_buffer_allocs_free(rage_BufferAllocs * const alloc);

rage_BuffersInfo const * rage_buffer_allocs_get_buffers_info(
    rage_BufferAllocs const * const alloc);
