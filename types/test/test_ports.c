#include "error.h"
#include "ports.h"
#include <stdlib.h>

rage_Error test_ports() {
    // TODO: come up with something more than execution for this?
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
    rage_Error rval = RAGE_OK;
    if (ports.inputs == NULL) {
        rval = RAGE_ERROR("Input array points to null");
    }
    rage_ports_free(ports);
    return rval;
}
