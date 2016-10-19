#include <stdlib.h>
#include "loader.h"

rage_ElementLoader rage_element_loader_new() {
    return NULL;
}

void rage_element_loader_free(rage_ElementLoader el) {
}

// FIXME: Dynamism
static char amp[] = "amp";
static char * element_types[] = {amp};

rage_ElementTypes rage_element_loader_list(rage_ElementLoader el) {
    return (rage_ElementTypes) {.len=1, .items=element_types};
}
