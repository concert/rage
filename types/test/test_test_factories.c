#include <string.h>
#include "atoms.h"
#include "test_factories.h"
#include "error.h"
#include "testing.h"

rage_Error test_time_series_new() {
    const rage_AtomDef int_type = {
        .type = RAGE_ATOM_INT,
        .name = "inty",
        .constraints = {.i = (rage_IntConstraints) {
             .min = RAGE_JUST(12), .max = RAGE_NOTHING
        }}
    };
    const rage_AtomDef string_type = {
        .type = RAGE_ATOM_STRING,
        .name = "stringy",
        .constraints = {.s = (rage_StringConstraints) RAGE_NOTHING}
    };
    const rage_FieldDef defs[] = {
        {.name = "wheat", .type = &int_type},
        {.name = "barley", .type = &string_type}
    };
    const rage_TupleDef td = {
        .name = "basil",
        .description = "herb",
        .default_value = NULL,
        .items = defs,
        .len = 2
    };
    rage_TimeSeries ts = rage_time_series_new(&td);
    rage_Error rval = RAGE_OK;
    if (ts.len != 1) {
        rval = RAGE_ERROR("timeseries is not 1 point long");
    } else if (ts.items == NULL) {
        rval = RAGE_ERROR("timeseries items is NULL");
    } else {
        rage_TimePoint * tp = ts.items;
        if (tp->time.second || tp->time.fraction) {
            rval = RAGE_ERROR("timeseries does not start at t=0");
        } else if (tp->mode != RAGE_INTERPOLATION_CONST) {
            rval = RAGE_ERROR("final time point has non-constant interpolation");
        } else if (tp->value[0].i != 12) {
            rval = RAGE_ERROR("integer has taken unexpected value");
        } else if (strcmp(tp->value[1].s, "")) {
            rval = RAGE_ERROR("wasn't an empty string");
        }
    }
    rage_time_series_free(ts);
    return rval;
}

TEST_MAIN(test_time_series_new)
