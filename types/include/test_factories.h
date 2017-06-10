#pragma once

/*
 * Makes a single time point time series based on the given discription.
 */
rage_TimeSeries rage_time_series_new(rage_TupleDef const * const item_def);

/*
 * Free a heap allocated time series.
 */
void rage_time_series_free(rage_TimeSeries ts);
