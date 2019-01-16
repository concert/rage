#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include "queue.h"

typedef struct rage_QueueItem {
    struct rage_QueueItem * next;
    rage_Event * evt;
} rage_QueueItem;

struct rage_Queue {
    sem_t s;
    pthread_mutex_t l;
    rage_QueueItem * head;
    rage_QueueItem * tail;
};

rage_Queue * rage_queue_new() {
    rage_Queue * q = malloc(sizeof(rage_Queue));
    sem_init(&q->s, 0, 0);
    pthread_mutex_init(&q->l, NULL);
    q->head = q->tail = NULL;
    return q;
}

void rage_queue_free(rage_Queue * q) {
    sem_destroy(&q->s);
    pthread_mutex_destroy(&q->l);
    free(q);
}

rage_QueueItem * rage_queue_item_new(rage_Event * evt) {
    rage_QueueItem * i = malloc(sizeof(rage_QueueItem));
    i->next = NULL;
    i->evt = evt;
    return i;
}

static void do_put(rage_Queue * queue, rage_QueueItem * item) {
    if (queue->tail) {
        queue->tail->next = item;
    } else {
        queue->head = item;
    }
    queue->tail = item;
    pthread_mutex_unlock(&queue->l);
    sem_post(&queue->s);
}

int rage_queue_put_nonblock(rage_Queue * queue, rage_QueueItem * item) {
    if (pthread_mutex_trylock(&queue->l) == EBUSY) {
        return 0;
    } else {
        do_put(queue, item);
        return 1;
    }
}

void rage_queue_put_block(rage_Queue * queue, rage_QueueItem * item) {
    pthread_mutex_lock(&queue->l);
    do_put(queue, item);
}

rage_Event * rage_queue_get_block(rage_Queue * queue) {
    sem_wait(&queue->s);
    pthread_mutex_lock(&queue->l);
    rage_QueueItem * i = queue->head;
    queue->head = i->next;
    if (queue->tail == i) {
        queue->tail = NULL;
    }
    pthread_mutex_unlock(&queue->l);
    rage_Event * e = i->evt;
    free(i);
    return e;
}
