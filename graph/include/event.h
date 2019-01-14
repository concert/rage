#pragma once

typedef enum {
    RAGE_EVENT_STOPPED,
    RAGE_EVENT_SRT_UNDERRUN,
    RAGE_EVENT_CLEAR_FAILED,
    RAGE_EVENT_PREP_FAILED,
    RAGE_EVENT_CLEAN_FAILED
} rage_EventType;

typedef struct rage_Event {
    rage_EventType event;
    char const * msg;
} rage_Event;

void rage_event_free(rage_Event * evt);
