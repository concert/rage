#pragma once
#include "element.h"

rage_TupleDef const init_params;
rage_PortDescription * elem_describe_ports(rage_Tuple params);
rage_NewElementState elem_new(
    uint32_t sample_rate, uint32_t frame_size, rage_Tuple params);
void elem_free(void * state);
rage_Error elem_process(void * state, rage_Time time, rage_Port * ports);


// FIXME: there has got to be a better way/place than this
rage_ElementGetPortsDescription rage_internal_check_pd = elem_describe_ports;
rage_ElementStateNew rage_internal_check_esn = elem_new;
rage_ElementStateFree rage_internal_check_esf = elem_free;
rage_ElementProcess rage_internal_check_p = elem_process;
