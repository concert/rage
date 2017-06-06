#include "ports.h"
#include <stdlib.h>

int main() {
    // FIXME: This isn't really a test.
    rage_AtomDef const unconstrained_float = {
        .type = RAGE_ATOM_FLOAT, .name = "float", .constraints = {}
    };
    rage_FieldDef fields[] = {
        {.name = "f_field", .type = &unconstrained_float}
    };
    rage_TupleDef td = {
        .name = "Ports Test",
        .description = "Testing's great",
        .len = 1,
        .items = fields
    };
    rage_InstanceSpec pd;
    pd.controls.items = &td;
    pd.controls.len = 1;
    pd.inputs.len = 2;
    pd.outputs.len = 3;
    rage_Ports ports = rage_ports_new(&pd);
    rage_ports_free(ports);
}
