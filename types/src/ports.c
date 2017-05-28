#include <stdlib.h>
#include "ports.h"

rage_Ports rage_ports_new(rage_InstanceSpec const * const spec) {
    rage_Ports ports;
    ports.controls = calloc(spec->controls.len, sizeof(rage_InterpolatedView *));
    ports.inputs = calloc(spec->inputs.len, sizeof(rage_InStream));
    ports.outputs = calloc(spec->outputs.len, sizeof(rage_OutStream));
    return ports;
}

void rage_ports_free(rage_Ports ports) {
    free(ports.inputs);
    free(ports.outputs);
    free(ports.controls);
}
