#pragma once
#include "error.h"
#include "countdown.h"
#include "interpolation.h"
#include "loader.h"
#include "queue.h"
#include "event.h"

extern rage_EventType * rage_EventSrtClearFailed;
extern rage_EventType * rage_EventSrtPrepFailed;
extern rage_EventType * rage_EventSrtCleanFailed;
extern rage_EventType * rage_EventSrtUnderrun;

typedef struct rage_SupportConvoy rage_SupportConvoy;
typedef struct rage_SupportTruck rage_SupportTruck;

rage_SupportConvoy * rage_support_convoy_new(
    uint32_t period_size, rage_TransportState transp_state,
    rage_Queue * evt_q);
void rage_support_convoy_free(rage_SupportConvoy * convoy);

rage_Countdown * rage_support_convoy_get_countdown(rage_SupportConvoy const * convoy);

rage_Error rage_support_convoy_start(rage_SupportConvoy * convoy);
rage_Error rage_support_convoy_stop(rage_SupportConvoy * convoy);

rage_SupportTruck * rage_support_convoy_mount(
    rage_SupportConvoy * convoy, rage_Element * elem,
    rage_InterpolatedView ** prep_views,
    rage_InterpolatedView ** clean_views);

void rage_support_convoy_unmount(rage_SupportTruck * truck);

void rage_support_convoy_transport_state_changing(
    rage_SupportConvoy * convoy, rage_TransportState state);
void rage_support_convoy_transport_state_changed(
    rage_SupportConvoy * convoy, rage_TransportState state);
rage_Error rage_support_convoy_transport_seek(
    rage_SupportConvoy * convoy, rage_FrameNo target_frame);
