#include <stdio.h>
#include <stdlib.h>
#include "loader.h"
#include <assert.h>

rage_Tuple generate_valid_tuple(rage_TupleDef const * td) {
    rage_Tuple tup = calloc(td->len, sizeof(rage_Atom));
    for (unsigned i=0; i < td->len; i++) {
        rage_AtomDef const * const at = td->items[i].type;
        switch (at->type) {
            case RAGE_ATOM_INT: {
                int32_t v = 0;
                if (at->constraints.i.min.half == RAGE_EITHER_RIGHT)
                    v = at->constraints.i.min.right;
                if (at->constraints.i.max.half == RAGE_EITHER_RIGHT)
                    v = at->constraints.i.max.right;
                tup[i].i = v;
                break;
            }
            case RAGE_ATOM_FLOAT: {
                float v = 0;
                if (at->constraints.f.min.half == RAGE_EITHER_RIGHT)
                    v = at->constraints.f.min.right;
                if (at->constraints.f.max.half == RAGE_EITHER_RIGHT)
                    v = at->constraints.f.max.right;
                tup[i].f = v;
                break;
            }
            case RAGE_ATOM_TIME:
            case RAGE_ATOM_STRING:
                assert(false);
        }
    }
    return tup;
}

typedef RAGE_ARRAY(rage_Port) rage_PortArray;

rage_PortArray create_test_ports(rage_PortDescription const * port_desc) {
    uint32_t n_ports = rage_port_description_count(port_desc);
    rage_Port * ports = calloc(n_ports, sizeof(rage_Port));
    unsigned port_idx = 0;
    for (
            rage_PortDescription const * pd = port_desc;
            pd != NULL;
            pd = pd->next) {
        rage_Port * port = &ports[port_idx++];
        port->desc = pd;
        switch (pd->type) {
            case (RAGE_PORT_STREAM):
                switch (pd->stream_def) {
                    case (RAGE_STREAM_AUDIO):
                        // FIXME: magic constant
                        port->out.samples = calloc(256, sizeof(float));
                        break;
                }
                break;
            case (RAGE_PORT_EVENT):
                // FIXME: magic const for no of events
                port->out.events = rage_event_frame_new(
                    pd->event_def.len * 8 * sizeof(rage_Atom));
                // FIXME: Hideous empty event thing below should die:
                port->out.events->events.len = 1;
                port->out.events->events.items = (rage_TimePoint *) port->out.events->buffer;
                break;
        }
    }
    return (rage_PortArray) {.items=ports, .len=n_ports};
}

void free_test_ports(rage_PortArray ports) {
    for (unsigned i=0; i < ports.len; i++) {
        rage_Port * port = &ports.items[i];
        switch (port->desc->type) {
            case (RAGE_PORT_STREAM):
                free(port->out.samples);
                break;
            case (RAGE_PORT_EVENT):
                rage_event_frame_free(port->out.events);
                break;
        }
    }
}


rage_Error test() {
    rage_ElementLoader el = rage_element_loader_new();
    rage_ElementTypes element_type_names = rage_element_loader_list(el);
    for (unsigned i=0; i<element_type_names.len; i++) {
        rage_ElementTypeLoadResult et_ = rage_element_loader_load(
            el, element_type_names.items[i]);
        RAGE_EXTRACT_VALUE(rage_Error, et_, rage_ElementType * et)
        rage_Tuple tup = generate_valid_tuple(et->parameters);
        rage_ElementNewResult elem_ = rage_element_new(et, 44100, 256, tup);
        RAGE_EXTRACT_VALUE(rage_Error, elem_, rage_Element * elem)
        rage_PortArray ports = create_test_ports(elem->ports);
        rage_element_process(elem, (rage_Time) {.second=0}, ports.items);
        free_test_ports(ports);
        rage_element_free(elem);
        free(tup);
        rage_element_loader_unload(el, et);
    }
    rage_element_loader_free(el);
    RAGE_OK
}
