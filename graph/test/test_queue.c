#include "queue.h"

struct rage_Event {
    int val;
};

static rage_Error test_queue() {
    rage_Event e0 = {.val = 0};
    rage_Event e1 = {.val = 1};
    rage_Event e2 = {.val = 2};
    rage_Error rv = RAGE_OK;
    rage_Queue * q = rage_queue_new();
    rage_QueueItem * i0 = rage_queue_item_new(&e0);
    rage_QueueItem * i1 = rage_queue_item_new(&e1);
    rage_QueueItem * i2 = rage_queue_item_new(&e2);
    rage_queue_put_block(q, i0);
    if (!rage_queue_put_nonblock(q, i1)) {
        return RAGE_ERROR("Nonblock put failed");
    }
    if (rage_queue_get_block(q)->val != 0) {
        return RAGE_ERROR("First item did not have val 0");
    }
    if (rage_queue_get_block(q)->val != 1) {
        return RAGE_ERROR("Second item did not have val 1");
    }
    rage_queue_put_block(q, i2);
    if (rage_queue_get_block(q)->val != 2) {
        return RAGE_ERROR("Third item did not have val 2");
    }
    rage_queue_free(q);
    return rv;
}
