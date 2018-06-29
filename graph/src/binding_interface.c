#include "binding_interface.h"

void rage_backend_get_buffers(
        rage_BackendInterface const * b, void ** inputs, void ** outputs) {
    return b->get_buffers(b->state, inputs, outputs);
}
