#pragma once
#include "error.h"
#include "loader.h"

typedef struct rage_Engine rage_Engine;
typedef RAGE_OR_ERROR(rage_Engine *) rage_NewEngine;

rage_NewEngine rage_engine_new();
void rage_engine_free(rage_Engine * engine);

rage_Error rage_engine_start(rage_Engine * engine);
rage_Error rage_engine_stop(rage_Engine * engine);

typedef struct rage_Harness rage_Harness;
typedef RAGE_OR_ERROR(rage_Harness *) rage_MountResult;
rage_MountResult rage_engine_mount(
    rage_Engine * engine, rage_Element * elem, rage_TimeSeries * controls,
    char const * name);
void rage_engine_unmount(rage_Harness * harness);
rage_Error rage_harness_set_time_series(
    rage_Harness * const harness, rage_TimeSeries * const new_time_series);
