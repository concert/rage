#include <stdlib.h>
#include "ports.h"

rage_PortDescription * rage_port_description_copy(rage_PortDescription pd) {
    rage_PortDescription * npd = malloc(sizeof(rage_PortDescription));
    npd->is_input = pd.is_input;
    npd->type = pd.type;
    npd->next = pd.next;
    switch (pd.type) {
        case (RAGE_PORT_STREAM):
            npd->stream_def = pd.stream_def;
            break;
        case (RAGE_PORT_EVENT):
            npd->event_def = pd.event_def;
            break;
    }
    return npd;
}

void rage_port_description_free(rage_PortDescription * const pdp) {
    free(pdp);
}
