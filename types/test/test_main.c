#include "testing.h"

#include "test_interpolation.c"
#include "test_ports.c"
#include "test_test_factories.c"
#include "test_chronology.c"
#include "test_queue.c"

TEST_MAIN(
    interpolator_float_test,
    interpolator_int_test,
    interpolator_time_test,
    interpolator_new_with_no_timepoints,
    interpolator_new_first_timepoint_not_start,
    interpolator_ambiguous_interpolation_mode,
    interpolator_timeseries_not_monotonic,
    interpolator_immediate_change_test,
    interpolator_delayed_change_test,
    interpolator_change_during_interpolation_test,
    test_ports,
    test_time_series_new,
    test_time_after,
    test_queue)
