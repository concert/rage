#pragma once
#include "error.h"
#include "binding_interface.h"
#include "chronology.h"

typedef struct rage_BackendState rage_JackBackend;
typedef RAGE_OR_ERROR(rage_JackBackend *) rage_NewJackBackend;

typedef RAGE_ARRAY(char *) rage_PortNames;

rage_NewJackBackend rage_jack_backend_new(
    uint32_t sample_rate, uint32_t buffer_size,
    rage_PortNames inputs, rage_PortNames outputs);
void rage_jack_backend_free(rage_JackBackend * jbe);

rage_Time rage_jack_backend_nowish(rage_JackBackend * jbe);

rage_BackendInterface * rage_jack_backend_get_interface(rage_JackBackend * jbe);
