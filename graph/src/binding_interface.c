#include "binding_interface.h"

rage_BackendHooks rage_backend_setup_process(
        rage_BackendInterface * bi,
        void * data,
        rage_ProcessCb process,
        rage_SetExternalsCb set_externals,
        uint32_t * buffer_size) {
    return bi->setup_process(
        bi->state, data, process, set_externals, buffer_size);
}

uint32_t rage_backend_get_sample_rate(rage_BackendInterface * bi) {
    return *bi->sample_rate;
}

void rage_backend_unset_process(rage_BackendInterface * bi) {
    bi->unset_process(bi->state);
}

rage_Error rage_backend_activate(rage_BackendInterface * bi) {
    return bi->activate(bi->state);
}

rage_Error rage_backend_deactivate(rage_BackendInterface * bi) {
    return bi->deactivate(bi->state);
}
