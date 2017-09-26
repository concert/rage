#include "srt.h"
#include "macros.h"
#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

struct rage_SupportTruck {
    rage_SupportConvoy * convoy;  // Not totally convinced by the backref
    rage_Element * elem;
    rage_InterpolatedView ** prep_view;
    rage_InterpolatedView ** clean_view;
};

typedef RAGE_ARRAY(rage_SupportTruck *) rage_Trucks;
static RAGE_POINTER_ARRAY_APPEND_FUNC_DEF(rage_Trucks, rage_SupportTruck, truck_append)
static RAGE_POINTER_ARRAY_REMOVE_FUNC_DEF(rage_Trucks, rage_SupportTruck, truck_remove)

struct rage_SupportConvoy {
    bool running;
    pthread_t worker_thread;
    uint32_t period_size;
    uint32_t invalid_after;
    rage_Countdown * countdown;
    // mutex to prevent truck array freeing during iteration:
    pthread_mutex_t active;
    rage_Trucks * prep_trucks;
    rage_Trucks * clean_trucks;
};

rage_SupportConvoy * rage_support_convoy_new(uint32_t period_size, rage_Countdown * countdown) {
    rage_SupportConvoy * convoy = malloc(sizeof(rage_SupportConvoy));
    convoy->running = false;
    convoy->period_size = period_size;
    convoy->invalid_after = UINT32_MAX;
    // FIXME: can in theory fail
    pthread_mutex_init(&convoy->active, NULL);
    convoy->countdown = countdown;
    convoy->prep_trucks = malloc(sizeof(rage_Trucks));
    convoy->prep_trucks->len = 0;
    convoy->prep_trucks->items = NULL;
    convoy->clean_trucks = malloc(sizeof(rage_Trucks));
    convoy->clean_trucks->len = 0;
    convoy->clean_trucks->items = NULL;
    return convoy;
}

void rage_support_convoy_free(rage_SupportConvoy * convoy) {
    pthread_mutex_destroy(&convoy->active);
    assert(convoy->prep_trucks->len == 0);
    assert(convoy->clean_trucks->len == 0);
    assert(!convoy->running);
    RAGE_ARRAY_FREE(convoy->prep_trucks);
    RAGE_ARRAY_FREE(convoy->clean_trucks);
    free(convoy);
}

static rage_PreparedFrames prep_truck(rage_SupportTruck * truck) {
    return RAGE_ELEM_PREP(truck->elem, truck->prep_view);
}

static rage_PreparedFrames clean_truck(rage_SupportTruck * truck) {
    return RAGE_ELEM_CLEAN(truck->elem, truck->clean_view);
}

static rage_PreparedFrames apply_to_trucks(
        rage_Trucks * trucks, rage_PreparedFrames (*op)(rage_SupportTruck *)) {
    uint32_t min_prepared = UINT32_MAX;
    for (unsigned i = 0; i < trucks->len; i++) {
        rage_PreparedFrames prepared = op(trucks->items[i]);
        if (RAGE_FAILED(prepared))
            return RAGE_FAILURE_CAST(rage_PreparedFrames, prepared);
        uint32_t n_prepared = RAGE_SUCCESS_VALUE(prepared);
        min_prepared = (n_prepared < min_prepared) ? n_prepared : min_prepared;
    }
    return RAGE_SUCCESS(rage_PreparedFrames, min_prepared);
}

static rage_Error clear(rage_Trucks * trucks, uint32_t preserve) {
    for (unsigned i = 0; i < trucks->len; i++) {
        rage_SupportTruck * truck = trucks->items[i];
        rage_Error err = RAGE_ELEM_CLEAR(truck->elem, truck->prep_view, preserve);
        if (RAGE_FAILED(err)) {
            return err;
        }
    }
    return RAGE_OK;
}

static void * rage_support_convoy_worker(void * ptr) {
    rage_SupportConvoy * convoy = ptr;
    char const * err = NULL;
    uint32_t min_frames_wait = UINT32_MAX;
    rage_PreparedFrames frames_until_next;
    uint32_t n_frames_until_next;
    pthread_mutex_lock(&convoy->active);
    #define RAGE_BAIL_ON_FAIL \
        if (RAGE_FAILED(frames_until_next)) { \
            err = RAGE_FAILURE_VALUE(frames_until_next); \
            break; \
        } else
    while (convoy->running) {
        if (convoy->invalid_after != UINT32_MAX) {
             clear(convoy->prep_trucks, convoy->invalid_after);
        }
        frames_until_next = apply_to_trucks(convoy->prep_trucks, prep_truck);
        RAGE_BAIL_ON_FAIL {
            n_frames_until_next = RAGE_SUCCESS_VALUE(frames_until_next);
            min_frames_wait = (min_frames_wait < n_frames_until_next) ?
                min_frames_wait : n_frames_until_next;
        }
        pthread_mutex_unlock(&convoy->active);
        if (0 > rage_countdown_add(
                convoy->countdown, min_frames_wait / convoy->period_size)) {
            err = "SupportConvoy failed to keep up";
            break;
        }
        rage_countdown_timed_wait(convoy->countdown, UINT32_MAX);
        pthread_mutex_lock(&convoy->active);
        frames_until_next = apply_to_trucks(convoy->clean_trucks, clean_truck);
        RAGE_BAIL_ON_FAIL {
            min_frames_wait = RAGE_SUCCESS_VALUE(frames_until_next);
        }
    }
    #undef RAGE_BAIL_ON_FAIL
    pthread_mutex_unlock(&convoy->active);
    return (void *) err;
}

rage_Error rage_support_convoy_start(rage_SupportConvoy * convoy) {
    assert(!convoy->running);
    convoy->running = true;
    if (pthread_create(
            &convoy->worker_thread, NULL,
            rage_support_convoy_worker, convoy)) {
        return RAGE_ERROR("Failed to create worker thread");
    }
    return RAGE_OK;
}

rage_Error rage_support_convoy_stop(rage_SupportConvoy * convoy) {
    assert(convoy->running);
    pthread_mutex_lock(&convoy->active);
    convoy->running = false;
    rage_countdown_unblock_wait(convoy->countdown);
    pthread_mutex_unlock(&convoy->active);
    char const * err;
    if (pthread_join(convoy->worker_thread, (void **) &err)) {
        return RAGE_ERROR("Failed to join worker thread");
    }
    if (err != NULL) {
        return RAGE_ERROR(err);
    }
    return RAGE_OK;
}

static void change_trucks(
        rage_Trucks * (next_trucks)(rage_Trucks *, rage_SupportTruck * truck),
        rage_SupportConvoy * convoy,
        rage_SupportTruck * truck) {
    // FIXME: This is assuming a single control thread
    rage_Trucks
        * old_prep_trucks,
        * new_prep_trucks,
        * old_clean_trucks,
        * new_clean_trucks;
    if (truck->prep_view) {
        old_prep_trucks = convoy->prep_trucks;
        new_prep_trucks = next_trucks(old_prep_trucks, truck);
    } else {
        old_prep_trucks = NULL;
        new_prep_trucks = convoy->prep_trucks;
    }
    if (truck->clean_view) {
        old_clean_trucks = convoy->clean_trucks;
        new_clean_trucks = next_trucks(old_clean_trucks, truck);
    } else {
        old_clean_trucks = NULL;
        new_clean_trucks = convoy->clean_trucks;
    }
    pthread_mutex_lock(&convoy->active);
    convoy->prep_trucks = new_prep_trucks;
    convoy->clean_trucks = new_clean_trucks;
    pthread_mutex_unlock(&convoy->active);
    if (old_prep_trucks != NULL) {
        RAGE_ARRAY_FREE(old_prep_trucks);
    }
    if (old_clean_trucks != NULL) {
        RAGE_ARRAY_FREE(old_clean_trucks);
    }
}

rage_SupportTruck * rage_support_convoy_mount(
        rage_SupportConvoy * convoy, rage_Element * elem,
        rage_InterpolatedView ** prep_view,
        rage_InterpolatedView ** clean_view) {
    rage_SupportTruck * truck = malloc(sizeof(rage_SupportTruck));
    truck->elem = elem;
    truck->prep_view = prep_view;
    truck->clean_view = clean_view;
    truck->convoy = convoy;
    change_trucks(truck_append, convoy, truck);
    return truck;
}

void rage_support_convoy_unmount(rage_SupportTruck * truck) {
    change_trucks(truck_remove, truck->convoy, truck);
    free(truck);
}

void rage_support_convoy_set_transport_state(
        rage_SupportConvoy * convoy, rage_TransportState state) {
    pthread_mutex_lock(&convoy->active);
    // FIXME: Doesn't do anything more than sync up
    pthread_mutex_unlock(&convoy->active);
}
