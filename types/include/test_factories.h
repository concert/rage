#pragma once
#include "time_series.h"

/*
 * Make a tuple meeting the constraints of the description..
 */
rage_Atom * rage_tuple_generate(rage_TupleDef const * const td);

/*
 * Makes a single time point time series based on the given discription.
 */
rage_TimeSeries rage_time_series_new(rage_TupleDef const * const item_def);

/*
 * Free a heap allocated time series.
 */
void rage_time_series_free(rage_TimeSeries ts);
