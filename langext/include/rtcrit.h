#pragma once

typedef struct rage_RtCrit rage_RtCrit;

rage_RtCrit * rage_rt_crit_new(void * initial_data);
void rage_rt_crit_free(rage_RtCrit * c);
void * rage_rt_crit_data_latest(rage_RtCrit * crit);
void * rage_rt_crit_data_update(rage_RtCrit * crit, void * next_data);
