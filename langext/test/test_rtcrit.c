#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>
#include "rtcrit.h"

typedef struct {
    rage_RtCrit * crit;
    sem_t main_sem;
    bool volatile processing;
    void * volatile process_data;
} rage_NonBlockTestData;

void * faux_rt_process(void * data) {
    rage_NonBlockTestData * td = data;
    bool first_iter = true;
    bool fire_on_change = true;
    while (td->processing) {
        td->process_data = rage_rt_crit_data_latest(td->crit);
        if (first_iter) {
            sem_post(&td->main_sem);
            first_iter = false;
        } else if (fire_on_change && td->process_data == NULL) {
            sem_post(&td->main_sem);
            fire_on_change = false;
        }
    }
    return NULL;
}

rage_Error test_rtcrit() {
    rage_Error err = RAGE_OK;
    int i;
    rage_NonBlockTestData td = {
        .crit = rage_rt_crit_new(&i),
        .processing = true,
        .process_data = NULL
    };
    sem_init(&td.main_sem, 0, 0);
    pthread_t process_thread;
    int thread_create_result = pthread_create(
        &process_thread, NULL, faux_rt_process, &td);
    if (!thread_create_result) {
        sem_wait(&td.main_sem);
        if (td.process_data != &i) {
            err = RAGE_ERROR("Initial data not received by proc thread");
        } else {
            int * p = rage_rt_crit_data_update(td.crit, NULL);
            sem_wait(&td.main_sem);
            if (p != &i) {
                err = RAGE_ERROR("Unexpected old ptr val");
            } else if (td.process_data != NULL) {
                err = RAGE_ERROR("Update failed");
            }
        }
        td.processing = false;
        pthread_join(process_thread, NULL);
    }
    sem_destroy(&td.main_sem);
    rage_rt_crit_free(td.crit);
    return err;
}
