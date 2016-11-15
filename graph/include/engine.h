#pragma once
#include "error.h"

typedef struct rage_Engine rage_Engine;
typedef RAGE_OR_ERROR(rage_Engine *) rage_NewEngine;

rage_NewEngine rage_engine_new();
void rage_engine_free(rage_Engine * engine);
