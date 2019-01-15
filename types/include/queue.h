#pragma once

typedef struct rage_Queue rage_Queue;
typedef struct rage_QueueItem rage_QueueItem;

rage_Queue * rage_queue_new();
void rage_queue_free(rage_Queue * queue);

rage_QueueItem * rage_queue_item_new(void * content);
void rage_queue_item_free(rage_QueueItem * qi);

int rage_queue_put_nonblock(rage_Queue * queue, rage_QueueItem * item);
void rage_queue_put_block(rage_Queue * queue, rage_QueueItem * item);
void * rage_queue_get_block(rage_Queue * queue);
