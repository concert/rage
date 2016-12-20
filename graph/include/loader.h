#pragma once

#include <stdint.h>
#include "atoms.h"
#include "element.h"
#include "error.h"
#include "macros.h"

typedef struct {
    rage_ElementType * type;
    void * state;
    union {
        rage_ProcessRequirements;
        rage_ProcessRequirements requirements;
    };
} rage_Element;

typedef struct rage_ElementLoader rage_ElementLoader;

rage_ElementLoader * rage_element_loader_new();
void rage_element_loader_free(rage_ElementLoader * el);

typedef RAGE_ARRAY(char *) rage_ElementTypes;
rage_ElementTypes rage_element_loader_list(rage_ElementLoader * el);
typedef RAGE_OR_ERROR(rage_ElementType *) rage_ElementTypeLoadResult;
rage_ElementTypeLoadResult rage_element_loader_load(
    rage_ElementLoader * el, char const * type_name);
void rage_element_loader_unload(
    rage_ElementLoader * el, rage_ElementType * type);

typedef RAGE_OR_ERROR(rage_Element *) rage_ElementNewResult;
rage_ElementNewResult rage_element_new(
    rage_ElementType * type, uint32_t sample_rate, uint32_t frame_size,
    rage_Atom * params);
void rage_element_free(rage_Element * elem);
rage_Error rage_element_process(
    rage_Element const * const elem, rage_TransportState const transport_state,
    rage_Ports const * ports);


// FIXME: better or worse?
#define RAGE_ELEM_PREP(elem) elem->type->prep(elem->state)
#define RAGE_ELEM_CLEAN(elem) elem->type->clean(elem->state)
#define RAGE_ELEM_CLEAR(elem) elem->type->clear(elem->state)
