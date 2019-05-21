#include "testing.h"
#include "test_array.c"
#include "test_countdown.c"
#include "test_set.c"
#include "test_rtcrit.c"

TEST_MAIN(test_countdown, test_array_init, test_array_append, test_array_remove, test_set, test_rtcrit)
