#pragma once
#include "error.h"
#include "loader.h"

typedef struct rage_JackBinding rage_JackBinding;
typedef RAGE_OR_ERROR(rage_JackBinding *) rage_NewJackBinding;

rage_NewJackBinding rage_jack_binding_new();
void rage_jack_binding_free(rage_JackBinding * jack_binding);

rage_Error rage_jack_binding_start(rage_JackBinding * jack_binding);
rage_Error rage_jack_binding_stop(rage_JackBinding * jack_binding);

typedef struct rage_JackHarness rage_JackHarness;
rage_JackHarness * rage_jack_binding_mount(
    rage_JackBinding * jack_binding, rage_Element * elem,
    rage_InterpolatedView ** rt_views, char const * name);
void rage_jack_binding_unmount(rage_JackHarness * harness);
