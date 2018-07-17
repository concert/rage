#pragma once

#include "macros.h"
#include "proc_block.h"

typedef RAGE_ARRAY(rage_Harness *) rage_HarnessArray;

typedef struct {
    rage_HarnessArray uncategorised;
    rage_HarnessArray in;
    rage_HarnessArray out;
    rage_HarnessArray rt;
} rage_CategorisedHarnesses;

rage_CategorisedHarnesses rage_categorise(
    rage_HarnessArray all_harnesses, rage_DepMap const * deps);
void rage_categorised_harnesses_free(rage_CategorisedHarnesses ch);
