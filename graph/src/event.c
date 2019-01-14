#include <stdlib.h>
#include "event.h"

void rage_event_free(rage_Event * evt) {
    free(evt);
}
