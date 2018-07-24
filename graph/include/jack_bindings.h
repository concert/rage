#pragma once
#include "error.h"
#include "binding_interface.h"
#include "chronology.h"

typedef struct rage_BackendState rage_JackBackend;
typedef RAGE_OR_ERROR(rage_JackBackend *) rage_NewJackBackend;

rage_NewJackBackend rage_jack_backend_new(rage_BackendConfig const conf);
void rage_jack_backend_free(rage_JackBackend * jbe);
rage_Error rage_jack_backend_activate(rage_JackBackend * jbe);
rage_Error rage_jack_backend_deactivate(rage_JackBackend * jbe);

rage_Time rage_jack_backend_nowish(rage_JackBackend * jbe);
