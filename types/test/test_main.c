#include "testing.h"

#include "test_interpolation.c"
#include "test_ports.c"
#include "test_test_factories.c"
#include "test_chronology.c"

TEST_MAIN(
    interpolator_float_test,
    interpolator_time_test,
    interpolator_immediate_change_test,
    test_ports,
    test_time_series_new,
    test_time_after)
