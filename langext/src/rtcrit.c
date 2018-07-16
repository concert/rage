#include "rtcrit.h"
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

struct rage_RtCrit {
    pthread_mutex_t mutex;
    sem_t * update_sem;
    void * rt_data;
    void * next_data;
};

rage_RtCrit * rage_rt_crit_new(void * initial_data) {
    rage_RtCrit * r = malloc(sizeof(rage_RtCrit));
    pthread_mutex_init(&r->mutex, NULL);
    r->update_sem = NULL;
    r->rt_data = initial_data;
    r->next_data = initial_data;
    return r;
}

void * rage_rt_crit_free(rage_RtCrit * c) {
    void * final_content = c->next_data;
    pthread_mutex_destroy(&c->mutex);
    if (c->update_sem != NULL) {
        sem_post(c->update_sem);
    }
    free(c);
    return final_content;
}

void * rage_rt_crit_data_latest(rage_RtCrit * crit) {
    if (!pthread_mutex_trylock(&crit->mutex)) {
        if (crit->rt_data != crit->next_data) {
            crit->rt_data = crit->next_data;
            sem_post(crit->update_sem);
            crit->update_sem = NULL;
        }
        pthread_mutex_unlock(&crit->mutex);
    }
    return crit->rt_data;
}

void const * rage_rt_crit_update_start(rage_RtCrit * crit) {
    pthread_mutex_lock(&crit->mutex);
    return crit->next_data;
}

void * rage_rt_crit_update_finish(rage_RtCrit * crit, void * next_data) {
    sem_t * our_sem = malloc(sizeof(sem_t));
    sem_init(our_sem, 0, 0);
    if (crit->update_sem != NULL) {
        sem_post(crit->update_sem);
    }
    crit->update_sem = our_sem;
    void * old_data = crit->next_data;
    crit->next_data = next_data;
    pthread_mutex_unlock(&crit->mutex);
    sem_wait(our_sem);
    sem_destroy(our_sem);
    free(our_sem);
    return old_data;
}

void rage_rt_crit_update_abort(rage_RtCrit * crit) {
    pthread_mutex_unlock(&crit->mutex);
}

void const * rage_rt_crit_freeze(rage_RtCrit * crit) {
    pthread_mutex_lock(&crit->mutex);
    return crit->rt_data;
}

void rage_rt_crit_thaw(rage_RtCrit * crit) {
    pthread_mutex_unlock(&crit->mutex);
}
