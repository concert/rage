#pragma once
#include "binding_interface.h"

typedef struct rage_BackendState rage_BulkBackend;

rage_BulkBackend * rage_bulk_backend_new(
    uint32_t const sample_rate, uint32_t const period_size);
void rage_bulk_backend_free(rage_BulkBackend * bb);

rage_BackendInterface * rage_bulk_backend_get_interface(rage_BulkBackend * bb);
