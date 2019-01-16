#include <stdio.h>
#include "testing.h"
#include "test_loader.c"
#include "test_jack_bindings.c"
#include "test_srt.c"
#include "test_graph.c"
#include "test_buffer_pile.c"
#include "test_categorisation.c"
#include "test_queue.c"

TEST_MAIN(
    test_loader, test_jack_bindings, test_srt, test_srt_fake_elem, test_graph,
    test_buffer_pile, test_categorise, test_queue)
