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
    char * err = NULL;
    if (ts.len != 1) {
        err = "timeseries is not 1 point long";
    }
    if ((err == NULL) && ts.items == NULL) {
        err = "timeseries items is NULL";
    }
    rage_TimePoint * tp = ts.items;
    if ((err == NULL) && (tp->time.second || tp->time.fraction)) {
        err = "timeseries does not start at t=0";
    }
    if ((err == NULL) && tp->mode != RAGE_INTERPOLATION_CONST) {
        err = "final time point has non-constant interpolation";
    }
    if ((err == NULL) && tp->value[0].i != 12) {
        err = "integer has taken unexpected value";
    }
    if ((err == NULL) && strcmp(tp->value[1].s, "")) {
        err = "wasn't an empty string";
    }
    rage_time_series_free(ts);
    if (err == NULL) {
        return RAGE_OK;
    } else {
        return RAGE_ERROR(err);  // FIXME: This structure is silly due to old implicit return
    }
}

TEST_MAIN(test_time_series_new)
