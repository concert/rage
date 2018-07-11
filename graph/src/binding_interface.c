#include "binding_interface.h"

void rage_backend_get_buffers(
        rage_BackendInterface const * b, uint32_t n_frames,
        void ** inputs, void ** outputs) {
    return b->get_buffers(b->state, n_frames, inputs, outputs);
}
