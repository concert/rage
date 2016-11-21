#include "ports.h"
#include <stdlib.h>

int main() {
    rage_PortDescription pd = {
        .is_input = true, .type = RAGE_PORT_STREAM,
        .stream_def = RAGE_STREAM_AUDIO, .next=NULL};
    rage_PortDescription * npd = rage_port_description_copy(pd);
    if (!npd->is_input)
        return 1;
    if (npd->type != RAGE_PORT_STREAM)
        return 2;
    if (npd->next != NULL)
        return 3;
    if (npd->stream_def != RAGE_STREAM_AUDIO)
        return 4;
    if (rage_port_description_count(npd) != 1)
        return 5;
}
