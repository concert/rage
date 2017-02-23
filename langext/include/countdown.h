#pragma once
#include "error.h"

typedef struct rage_Countdown rage_Countdown;

rage_Countdown * rage_countdown_new(int initial_value);
void rage_countdown_free(rage_Countdown * c);

rage_Error rage_countdown_timed_wait(rage_Countdown * c, unsigned millis);
int rage_countdown_add(rage_Countdown * c, int delta);
