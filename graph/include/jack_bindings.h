#pragma once
#include "error.h"
#include "loader.h"

typedef struct rage_JackBinding rage_JackBinding;
typedef RAGE_OR_ERROR(rage_JackBinding *) rage_NewJackBinding;

rage_NewJackBinding rage_jack_binding_new();
void rage_jack_binding_free(rage_JackBinding * engine);

rage_Error rage_jack_binding_start(rage_JackBinding * engine);
rage_Error rage_jack_binding_stop(rage_JackBinding * engine);

typedef struct rage_JackHarness rage_JackHarness;
typedef RAGE_OR_ERROR(rage_JackHarness *) rage_MountResult;
rage_MountResult rage_jack_binding_mount(
    rage_JackBinding * engine, rage_Element * elem, rage_TimeSeries * controls,
    char const * name);
void rage_jack_binding_unmount(rage_JackHarness * harness);
rage_Finaliser * rage_harness_set_time_series(
    rage_JackHarness * const harness,
    uint32_t const series_idx,
    rage_TimeSeries const * const new_controls);
