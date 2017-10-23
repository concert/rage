#include <stdio.h>
#include "testing.h"
#include "test_loader.c"
#include "test_jack_bindings.c"
#include "test_srt.c"
#include "test_graph.c"

TEST_MAIN(test_loader, test_jack_bindings, test_srt, test_srt_fake_elem, test_graph)
