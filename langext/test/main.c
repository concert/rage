#include "testing.h"
#include "test_array.c"
#include "test_synchronisation_primitives.c"
#include "test_set.c"

TEST_MAIN(test_countdown, test_array_init, test_array_append, test_array_remove, test_set)
