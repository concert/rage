#include "binding_interface.h"

rage_BackendHooks rage_backend_setup_process(
        rage_BackendInterface * bi,
        void * data,
        rage_ProcessCb process,
        rage_SetExternalsCb set_externals,
        uint32_t * sample_rate,
        uint32_t * buffer_size) {
    return bi->setup_process(
        bi->state, data, process, set_externals, sample_rate, buffer_size);
}
