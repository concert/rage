#include <stdlib.h>
#include "event.h"

struct rage_Event {
    rage_EventType * type;
    rage_EventId id;
    void * state;
    char const * (*msg)(void *);
    void (*free)(void *);
};

rage_Event * rage_event_new(
        rage_EventType * type,
        rage_EventId id,
        char const * (*msg)(void *),
        void (*free)(void *),
        void * state) {
    rage_Event * evt = malloc(sizeof(rage_Event));
    evt->type = type;
    evt->id = id;
    evt->state = state;
    evt->msg = msg;
    evt->free = free;
    return evt;
}

void rage_event_free(rage_Event * evt) {
    if (evt->free) {
        evt->free(evt->state);
    }
    free(evt);
}

rage_EventType * rage_event_type(rage_Event const * evt) {
    return evt->type;
}

char const * rage_event_msg(rage_Event const * evt) {
    if (evt->msg) {
        return evt->msg(evt->state);
    } else {
        return NULL;
    }
}

rage_EventId rage_event_id(rage_Event const * evt) {
    return evt->id;
}
