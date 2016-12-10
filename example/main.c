#include <stdio.h>
#include <unistd.h> // for sleep
#include "error.h"
#include "loader.h"
#include "harness.h"

int main() {
    printf("Example started\n");
    rage_NewEngine ne = rage_engine_new();
    if (RAGE_FAILED(ne)) {
        printf("Engine creation failed: %s\n", RAGE_FAILURE_VALUE(ne));
        return 1;
    }
    rage_Engine * const e = RAGE_SUCCESS_VALUE(ne);
    printf("Engine created\n");
    rage_ElementLoader el = rage_element_loader_new();
    //rage_ElementTypes element_type_names = rage_element_loader_list(el);
    // FIXME: loading super busted
    rage_ElementTypeLoadResult et_ = rage_element_loader_load(
        el, "./build/libamp.so");
    if (RAGE_FAILED(et_)) {
        printf("Element type load failed: %s\n", RAGE_FAILURE_VALUE(et_));
        rage_engine_free(e);
        return 2;
    }
    rage_ElementType * const et = RAGE_SUCCESS_VALUE(et_);
    printf("Element type loaded\n");
    // FIXME: Fixed stuff for amp
    rage_Atom tup[] = {
        {.i=1}
    };
    // FIXME: not working out frame size or anything
    rage_ElementNewResult elem_ = rage_element_new(et, 44100, 1024, tup);
    if (RAGE_FAILED(elem_)) {
        printf("Element load failed: %s\n", RAGE_FAILURE_VALUE(elem_));
        rage_engine_free(e);
        rage_element_loader_free(el);
        return 3;
    }
    rage_Element * const elem = RAGE_SUCCESS_VALUE(elem_);
    printf("Element loaded\n");
    rage_MountResult mr = rage_engine_mount(e, elem, "amp");
    if (RAGE_FAILED(mr)) {
        printf("Mount failed\n");
        rage_element_free(elem);
        rage_element_loader_unload(el, et);
        rage_element_loader_free(el);
        rage_engine_free(e);
        return 4;
    }
    rage_Harness * const harness = RAGE_SUCCESS_VALUE(mr);
    rage_Atom vals[] = {{.f = 0.01}};
    rage_TimePoint tps[] = {
        {
            .time = {.second = 0},
            .value = &(vals[0]),
            .mode = RAGE_INTERPOLATION_CONST
        }
    };
    rage_TimeSeries ts = {
        .len = 1,
        .items = tps
    };
    rage_harness_set_time_series(harness, &ts);
    //FIXME: handle errors (start/stop)
    printf("Starting engine...\n");
    rage_Error en_st = rage_engine_start(e);
    if (RAGE_FAILED(en_st)) {
        printf("Start failed: %s\n", RAGE_FAILURE_VALUE(en_st));
    } else {
        printf("Sleeping...\n");
        sleep(90);
        rage_engine_stop(e);
    }
    rage_engine_unmount(e, harness);
    printf("Unmounted\n");
    rage_element_free(elem);
    printf("Elem freed\n");
    rage_element_loader_unload(el, et);
    printf("Elem type freed\n");
    rage_element_loader_free(el);
    printf("Elem loader freed\n");
    rage_engine_free(e);
}
