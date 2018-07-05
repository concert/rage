#pragma once

typedef struct rage_RtCrit rage_RtCrit;

rage_RtCrit * rage_rt_crit_new(void * initial_data);
void * rage_rt_crit_free(rage_RtCrit * c);
void * rage_rt_crit_data_latest(rage_RtCrit * crit);

void const * rage_rt_crit_update_start(rage_RtCrit * crit);
void * rage_rt_crit_update_finish(rage_RtCrit * crit, void * next);

void const * rage_rt_crit_freeze(rage_RtCrit * crit);
void rage_rt_crit_thaw(rage_RtCrit * crit);
