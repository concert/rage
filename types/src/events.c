#include <stdlib.h>
#include "events.h"

rage_EventFrame * rage_event_frame_new(uint32_t capacity) {
    rage_EventFrame * const frame = malloc(sizeof(rage_EventFrame));
    frame->buffer = malloc(capacity);
    frame->capacity = capacity;
    frame->fill = 0;
    frame->events = (rage_TimeSeries) {.len = 0, .items = NULL};
    return frame;
}

void rage_event_frame_free(rage_EventFrame * const frame) {
    free(frame->buffer);
    free(frame);
}
