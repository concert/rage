#pragma once
#include "error.h"
#include "loader.h"
#include "countdown.h"

typedef struct rage_JackBinding rage_JackBinding;
typedef RAGE_OR_ERROR(rage_JackBinding *) rage_NewJackBinding;

rage_NewJackBinding rage_jack_binding_new(
    rage_Countdown * countdown, uint32_t * sample_rate);
void rage_jack_binding_free(rage_JackBinding * jack_binding);

rage_Error rage_jack_binding_start(rage_JackBinding * jack_binding);
rage_Error rage_jack_binding_stop(rage_JackBinding * jack_binding);

typedef struct rage_JackHarness rage_JackHarness;
rage_JackHarness * rage_jack_binding_mount(
    rage_JackBinding * jack_binding, rage_Element * elem,
    rage_InterpolatedView ** rt_views, char const * name);
void rage_jack_binding_unmount(rage_JackHarness * harness);

void rage_jack_binding_set_transport_state(
    rage_JackBinding * binding, rage_TransportState state);
