#include <stdlib.h>
#include "ports.h"

rage_Ports rage_ports_new(rage_ProcessRequirements const * const requirements) {
    rage_Ports ports;
    ports.controls.len = requirements->controls.len;
    ports.controls.items = calloc(requirements->controls.len, sizeof(rage_TimeSeries));
    for (uint32_t i = 0; i < requirements->controls.len; i++) {
        ports.controls.items[i] = rage_time_series_new(
            &requirements->controls.items[i]);
    }
    ports.inputs = calloc(requirements->inputs.len, sizeof(rage_InStream));
    ports.outputs = calloc(requirements->outputs.len, sizeof(rage_OutStream));
    return ports;
}

void rage_ports_free(rage_Ports ports) {
    for (uint32_t i = 0; i < ports.controls.len; i++) {
        rage_time_series_free(ports.controls.items[i]);
    }
    free(ports.inputs);
    free(ports.outputs);
    free(ports.controls.items);
}
