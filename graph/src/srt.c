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

typedef RAGE_ARRAY(rage_SupportTruck) rage_Trucks;

struct rage_SupportConvoy {
    bool running;
    pthread_t worker_thread;
    uint32_t period_size;
    uint32_t invalid_after;
    pthread_mutex_t active;
    rage_Countdown * countdown;
    rage_Trucks trucks;
};

rage_SupportConvoy * rage_support_convoy_new(uint32_t period_size, rage_Countdown * countdown) {
    rage_SupportConvoy * convoy = malloc(sizeof(rage_SupportConvoy));
    convoy->running = false;
    convoy->period_size = period_size;
    convoy->invalid_after = UINT32_MAX;
    // FIXME: can in theory fail
    pthread_mutex_init(&convoy->active, NULL);
    convoy->countdown = countdown;
    convoy->trucks.len = 0;
    convoy->trucks.items = NULL;
    return convoy;
}

void rage_support_convoy_free(rage_SupportConvoy * convoy) {
    pthread_mutex_destroy(&convoy->active);
    assert(convoy->trucks.len == 0);
    assert(!convoy->running);
    free(convoy);
}

static rage_PreparedFrames prepare(rage_Trucks * trucks) {
    uint32_t min_prepared = UINT32_MAX;
    for (unsigned i = 0; i < trucks->len; i++) {
        rage_SupportTruck * truck = trucks->items + i;
        if (truck->prep_view == NULL) {
            // FIXME: having to work out what to skip each iteration is inefficient
            continue;
        }
        rage_PreparedFrames prepared = RAGE_ELEM_PREP(truck->elem, truck->prep_view);
        RAGE_EXTRACT_VALUE(rage_PreparedFrames, prepared, uint32_t n_prepared)
        min_prepared = (n_prepared < min_prepared) ? n_prepared : min_prepared;
    }
    RAGE_SUCCEED(rage_PreparedFrames, min_prepared)
}

// FIXME: this is similar to prepare
static rage_PreparedFrames clean(rage_Trucks * trucks) {
    uint32_t min_prepared = UINT32_MAX;
    for (unsigned i = 0; i < trucks->len; i++) {
        rage_SupportTruck * truck = trucks->items + i;
        if (truck->clean_view == NULL) {
            // FIXME: having to work out what to skip each iteration is inefficient
            continue;
        }
        rage_PreparedFrames prepared = RAGE_ELEM_CLEAN(truck->elem, truck->clean_view);
        RAGE_EXTRACT_VALUE(rage_PreparedFrames, prepared, uint32_t n_prepared)
        min_prepared = (n_prepared < min_prepared) ? n_prepared : min_prepared;
    }
    RAGE_SUCCEED(rage_PreparedFrames, min_prepared)
}

static rage_Error clear(rage_Trucks * trucks, uint32_t preserve) {
    for (unsigned i = 0; i < trucks->len; i++) {
        rage_SupportTruck * truck = trucks->items + i;
        rage_Error err = RAGE_ELEM_CLEAR(truck->elem, truck->prep_view, preserve);
        if (RAGE_FAILED(err)) {
            return err;
        }
    }
    RAGE_OK
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
             clear(&convoy->trucks, convoy->invalid_after);
        }
        frames_until_next = prepare(&convoy->trucks);
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
        frames_until_next = clean(&convoy->trucks);
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
        RAGE_ERROR("Failed to create worker thread");
    }
    RAGE_OK
}

rage_Error rage_support_convoy_stop(rage_SupportConvoy * convoy) {
    assert(convoy->running);
    convoy->running = false;
    char const * err;
    if (pthread_join(convoy->worker_thread, (void **) &err)) {
        RAGE_ERROR("Failed to join worker thread")
    }
    if (err != NULL) {
        RAGE_ERROR(err)
    }
    RAGE_OK
}

// UGH, going to need a different handle as this can't be used to remove again
// (unless we go to trucks being a bunch of pointers, which might be less wierd
// anyway)
rage_SupportTruck * rage_support_convoy_mount(
        rage_SupportConvoy * convoy, rage_Element * elem,
        rage_InterpolatedView ** prep_view,
        rage_InterpolatedView ** clean_view) {
    uint32_t appended_len = convoy->trucks.len + 1;
    rage_SupportTruck * appended_items = calloc(appended_len, sizeof(rage_SupportTruck));
    memcpy(appended_items, convoy->trucks.items, sizeof(rage_SupportTruck) * convoy->trucks.len);
    rage_SupportTruck * truck = appended_items + convoy->trucks.len;
    truck->elem = elem;
    truck->prep_view = prep_view;
    truck->clean_view = clean_view;
    truck->convoy = convoy;
    pthread_mutex_lock(&convoy->active);
    convoy->trucks.items = appended_items;
    convoy->trucks.len = appended_len;
    pthread_mutex_unlock(&convoy->active);
    return truck;
}

void rage_support_convoy_unmount(rage_SupportTruck * truck) {
    // FIXME: just empties everything!
    pthread_mutex_lock(&truck->convoy->active);
    rage_SupportTruck * old_items = truck->convoy->trucks.items;
    truck->convoy->trucks.len = 0;
    truck->convoy->trucks.items = NULL;
    pthread_mutex_unlock(&truck->convoy->active);
    free(old_items);
}

void rage_support_convoy_set_transport_state(
        rage_SupportConvoy * convoy, rage_TransportState state) {
    pthread_mutex_lock(&convoy->active);
    // FIXME: Doesn't do anything more than sync up
    pthread_mutex_unlock(&convoy->active);
}
