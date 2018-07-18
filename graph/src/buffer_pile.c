#include "buffer_pile.h"
#include "macros.h"
#include <stdlib.h>

struct rage_BufferAllocs {
    rage_BufferAllocs * next;
    rage_BufferAllocs * prev;
    RAGE_ANON_UNION(rage_BuffersInfo, info);
};

rage_BufferAllocs * rage_buffer_allocs_new(size_t buffer_size) {
    rage_BufferAllocs * ba = malloc(sizeof(rage_BufferAllocs));
    ba->buffer_size = buffer_size;
    ba->next = NULL;
    ba->prev = NULL;
    ba->n_buffers = 0;
    ba->buffers = NULL;
    return ba;
}

static rage_BufferAllocs const * rage_buffer_allocs_max_alloc(
        rage_BufferAllocs const * allocs) {
    rage_BufferAllocs const * highest = NULL;
    for (unsigned max_n = 0; allocs != NULL; allocs = allocs->prev) {
        if (allocs->n_buffers >= max_n) {
            highest = allocs;
            max_n = allocs->n_buffers;
        }
    }
    return highest;
}

rage_BufferAllocs * rage_buffer_allocs_alloc(
        rage_BufferAllocs * const prev_allocs, unsigned const n_buffers) {
    rage_BufferAllocs const * largest_existing =
        rage_buffer_allocs_max_alloc(prev_allocs);
    char ** existing_buffers = largest_existing->buffers;
    unsigned n_existing_buffers = largest_existing->n_buffers;
    char ** new_buffs = calloc(n_buffers, sizeof(char *));
    for (unsigned i = 0; i < n_buffers; i++) {
        if (i < n_existing_buffers) {
            new_buffs[i] = existing_buffers[i];
        } else {
            new_buffs[i] = malloc(prev_allocs->buffer_size);
        }
    }
    rage_BufferAllocs * new_allocs = malloc(sizeof(rage_BufferAllocs));
    new_allocs->buffer_size = prev_allocs->buffer_size;
    new_allocs->next = NULL;
    new_allocs->prev = prev_allocs;
    prev_allocs->next = new_allocs;
    new_allocs->n_buffers = n_buffers;
    new_allocs->buffers = new_buffs;
    return new_allocs;
}

void rage_buffer_allocs_free(rage_BufferAllocs * const alloc) {
    unsigned i = 0;
    rage_BufferAllocs const * largest_remaining =
        rage_buffer_allocs_max_alloc(alloc->prev);
    if (largest_remaining != NULL) {
        i = largest_remaining->n_buffers;
    }
    for (rage_BufferAllocs * ba = alloc->next; ba != NULL; ba = ba->next) {
        i = (ba->n_buffers > i) ? ba->n_buffers : i;
    }
    if (alloc->next != NULL) {
        alloc->next->prev = alloc->prev;
    }
    if (alloc->prev != NULL) {
        alloc->prev->next = alloc->next;
    }
    for (; i < alloc->n_buffers; i++) {
        free(alloc->buffers[i]);
    }
    free(alloc->buffers);
    free(alloc);
}

rage_BuffersInfo const * rage_buffer_allocs_get_buffers_info(
        rage_BufferAllocs const * const alloc) {
    return &alloc->info;
}
