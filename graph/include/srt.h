#include "error.h"
#include "countdown.h"
#include "interpolation.h"
#include "loader.h"

typedef struct rage_SupportConvoy rage_SupportConvoy;
typedef struct rage_SupportTruck rage_SupportTruck;

rage_SupportConvoy * rage_support_convoy_new(uint32_t period_size, rage_Countdown * countdown);
void rage_support_convoy_free(rage_SupportConvoy * convoy);

rage_Error rage_support_convoy_start(rage_SupportConvoy * convoy);
rage_Error rage_support_convoy_stop(rage_SupportConvoy * convoy);

rage_SupportTruck * rage_support_convoy_mount(
    rage_SupportConvoy * convoy, rage_Element * elem,
    rage_InterpolatedView ** prep_view,
    rage_InterpolatedView ** clean_view);
