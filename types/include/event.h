#pragma once

typedef char const rage_EventType;
typedef struct rage_Event rage_Event;

rage_Event * rage_event_new(
    rage_EventType * event,
    char const * (*msg)(void *),
    void (*free)(void *),
    void * state);
void rage_event_free(rage_Event * evt);

rage_EventType * rage_event_type(rage_Event * evt);
char const * rage_event_msg(rage_Event * evt);
