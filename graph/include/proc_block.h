#pragma once
#include "error.h"
#include "loader.h"
#include "time_series.h"
#include "binding_interface.h"
#include "con_trans.h"
#include "queue.h"

typedef struct rage_ProcBlock rage_ProcBlock;
typedef struct rage_Harness rage_Harness;
typedef RAGE_OR_ERROR(rage_Harness *) rage_MountResult;

rage_ProcBlock * rage_proc_block_new(
    uint32_t sample_rate, uint32_t period_size,
    rage_BackendPorts ports, rage_TransportState transp_state, rage_Queue * evt_q);
void rage_proc_block_free(rage_ProcBlock * pb);

rage_Error rage_proc_block_start(rage_ProcBlock * pb);
rage_Error rage_proc_block_stop(rage_ProcBlock * pb);

rage_MountResult rage_proc_block_mount(
    rage_ProcBlock * pb, rage_Element * elem,
    rage_TimeSeries const * controls);
void rage_proc_block_unmount(rage_Harness * harness);

rage_NewEventId rage_harness_set_time_series(
    rage_Harness * const harness,
    uint32_t const series_idx,
    rage_TimeSeries const * const new_controls);

void rage_proc_block_set_transport_state(rage_ProcBlock * pb, rage_TransportState state);
rage_Error rage_proc_block_transport_seek(rage_ProcBlock * pb, rage_FrameNo target);

rage_ConTrans * rage_proc_block_con_trans_start(rage_ProcBlock * pb);
void rage_proc_block_con_trans_commit(rage_ConTrans * trans);
void rage_proc_block_con_trans_abort(rage_ConTrans * trans);
rage_Error rage_proc_block_connect(
    rage_ConTrans * trans,
    rage_Harness * source, uint32_t source_idx,
    rage_Harness * sink, uint32_t sink_idx);
rage_Error rage_proc_block_disconnect(
    rage_ConTrans * trans,
    rage_Harness * source, uint32_t source_idx,
    rage_Harness * sink, uint32_t sink_idx);

rage_BackendConfig rage_proc_block_get_backend_config(rage_ProcBlock * pb);
