#pragma once

#include <stdint.h>
#include "atoms.h"
#include "element.h"
#include "error.h"
#include "macros.h"
#include "ports.h"

// Am I exposing too much of the internals?
typedef struct {
    rage_ElementType * type;
    rage_Atom ** params;
    union {
        rage_InstanceSpec;
        rage_InstanceSpec spec;
    };
} rage_ConcreteElementType;

typedef struct {
    rage_ConcreteElementType * cet;
    rage_ElementState * state;
} rage_Element;

typedef struct rage_ElementLoader rage_ElementLoader;

rage_ElementLoader * rage_element_loader_new();
void rage_element_loader_free(rage_ElementLoader * el);

typedef RAGE_ARRAY(char *) rage_ElementTypes;
rage_ElementTypes * rage_element_loader_list(rage_ElementLoader * el);
void rage_element_types_free(rage_ElementTypes * types);

typedef RAGE_OR_ERROR(rage_ElementType *) rage_ElementTypeLoadResult;
rage_ElementTypeLoadResult rage_element_loader_load(
    rage_ElementLoader * el, char const * type_name);
void rage_element_loader_unload(
    rage_ElementLoader * el, rage_ElementType * type);

typedef RAGE_OR_ERROR(rage_ConcreteElementType *) rage_NewConcreteElementType;
rage_NewConcreteElementType rage_element_type_specialise(
    rage_ElementType * et, rage_Atom ** params);
void rage_concrete_element_type_free(rage_ConcreteElementType * ct);

typedef RAGE_OR_ERROR(rage_Element *) rage_ElementNewResult;
rage_ElementNewResult rage_element_new(
    rage_ConcreteElementType * type, uint32_t sample_rate, uint32_t frame_size);
void rage_element_free(rage_Element * elem);
void rage_element_process(
    rage_Element const * const elem, rage_TransportState const transport_state,
    rage_Ports const * ports);


// FIXME: better or worse?
#define RAGE_ELEM_PREP(elem, controls) elem->cet->type->prep(elem->state, controls)
#define RAGE_ELEM_CLEAN(elem, controls) elem->cet->type->clean(elem->state, controls)
#define RAGE_ELEM_CLEAR(elem, ...) elem->cet->type->clear(elem->state, __VA_ARGS__)
