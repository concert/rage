#include <stdlib.h>
#include "ports.h"

rage_Ports rage_ports_new(rage_ProcessRequirements const * const requirements) {
    rage_Ports ports;
    ports.controls = calloc(requirements->controls.len, sizeof(rage_Interpolator *));
    ports.inputs = calloc(requirements->inputs.len, sizeof(rage_InStream));
    ports.outputs = calloc(requirements->outputs.len, sizeof(rage_OutStream));
    return ports;
}

void rage_ports_free(rage_Ports ports) {
    free(ports.inputs);
    free(ports.outputs);
    free(ports.controls);
}
