#include "queue.h"

static rage_Error test_queue() {
    int e0 = 0, e1 = 1, e2 = 2;
    rage_Error rv = RAGE_OK;
    rage_Queue * q = rage_queue_new();
    rage_QueueItem * i0 = rage_queue_item_new(&e0);
    rage_QueueItem * i1 = rage_queue_item_new(&e1);
    rage_QueueItem * i2 = rage_queue_item_new(&e2);
    rage_queue_put_block(q, i0);
    if (!rage_queue_put_nonblock(q, i1)) {
        return RAGE_ERROR("Nonblock put failed");
    }
    if (*((int *) rage_queue_get_block(q)) != 0) {
        return RAGE_ERROR("First item did not have val 0");
    }
    if (*((int *) rage_queue_get_block(q)) != 1) {
        return RAGE_ERROR("Second item did not have val 1");
    }
    rage_queue_put_block(q, i2);
    if (*((int *) rage_queue_get_block(q)) != 2) {
        return RAGE_ERROR("Third item did not have val 2");
    }
    rage_queue_free(q);
    return rv;
}
