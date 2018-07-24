#pragma once

#include <stdint.h>
#include "atoms.h"
#include "element.h"
#include "error.h"
#include "macros.h"
#include "ports.h"

typedef struct rage_ElementKind rage_ElementKind;

// Am I exposing too much of the internals?
typedef struct {
    rage_ElementType * type;
    rage_Atom ** params;
    RAGE_EMBED_STRUCT(rage_InstanceSpec, spec);
} rage_ConcreteElementType;

typedef struct {
    rage_ConcreteElementType * cet;
    rage_ElementState * state;
} rage_Element;

typedef struct rage_ElementLoader rage_ElementLoader;

rage_ElementLoader * rage_element_loader_new(char const * elems_path);
void rage_element_loader_free(rage_ElementLoader * el);

// Hack around tests running before install
#include <dirent.h>
typedef int (*rage_PluginFilter)(struct dirent const * entry);
rage_ElementLoader * rage_fussy_element_loader_new(char const * elems_path, rage_PluginFilter pf);

typedef RAGE_ARRAY(char *) rage_ElementTypes;
rage_ElementTypes * rage_element_loader_list(rage_ElementLoader * el);
void rage_element_types_free(rage_ElementTypes * types);

typedef RAGE_OR_ERROR(rage_ElementKind *) rage_ElementKindLoadResult;
rage_ElementKindLoadResult rage_element_loader_load(char const * type_name);
void rage_element_loader_unload(rage_ElementKind * kind);
rage_ParamDefList const * rage_element_kind_parameters(rage_ElementKind const * kind);

typedef RAGE_OR_ERROR(rage_ConcreteElementType *) rage_NewConcreteElementType;
rage_NewConcreteElementType rage_element_type_specialise(
    rage_ElementKind * ek, rage_Atom ** params);
void rage_concrete_element_type_free(rage_ConcreteElementType * ct);

typedef RAGE_OR_ERROR(rage_Element *) rage_ElementNewResult;
rage_ElementNewResult rage_element_new(
    rage_ConcreteElementType * type, uint32_t sample_rate);
void rage_element_free(rage_Element * elem);
void rage_element_process(
    rage_Element const * const elem, rage_TransportState const transport_state,
    uint32_t period_size, rage_Ports const * ports);


// FIXME: better or worse?
#define RAGE_ELEM_PREP(elem, controls) elem->cet->type->prep(elem->state, controls)
#define RAGE_ELEM_CLEAN(elem, controls) elem->cet->type->clean(elem->state, controls)
#define RAGE_ELEM_CLEAR(elem, ...) elem->cet->type->clear(elem->state, __VA_ARGS__)
