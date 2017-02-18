#pragma once

typedef struct rage_Countdown rage_Countdown;

rage_Countdown * rage_countdown_new(int initial_value);
void rage_countdown_free(rage_Countdown * c);

void rage_countdown_wait(rage_Countdown * c);
int rage_countdown_add(rage_Countdown * c, int delta);
