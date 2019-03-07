#pragma once

#include <stdint.h>
#include "atoms.h"
#include "element.h"
#include "error.h"
#include "macros.h"
#include "ports.h"

typedef struct rage_LoadedElementKind rage_LoadedElementKind;

// Am I exposing too much of the internals?
typedef struct {
    rage_ElementType * type;
    rage_ElementState * state;
} rage_Element;

typedef struct rage_ElementLoader rage_ElementLoader;

rage_ElementLoader * rage_element_loader_new(char const * elems_path);
void rage_element_loader_free(rage_ElementLoader * el);

// Hack around tests running before install
#include <dirent.h>
typedef int (*rage_PluginFilter)(struct dirent const * entry);
rage_ElementLoader * rage_fussy_element_loader_new(char const * elems_path, rage_PluginFilter pf);

typedef RAGE_ARRAY(char *) rage_ElementKinds;
rage_ElementKinds * rage_element_loader_list(rage_ElementLoader * el);
void rage_element_kinds_free(rage_ElementKinds * kinds);

typedef RAGE_OR_ERROR(rage_LoadedElementKind *) rage_LoadedElementKindLoadResult;
rage_LoadedElementKindLoadResult rage_element_loader_load(char const * kind_name);
void rage_element_loader_unload(rage_LoadedElementKind * kind);
rage_ParamDefList const * rage_element_kind_parameters(rage_LoadedElementKind const * kind);

typedef RAGE_OR_ERROR(rage_ElementType *) rage_NewElementType;
rage_NewElementType rage_element_kind_specialise(
    rage_LoadedElementKind * ek, rage_Atom ** params);
void rage_element_type_free(rage_ElementType * et);

typedef RAGE_OR_ERROR(rage_Element *) rage_ElementNewResult;
rage_ElementNewResult rage_element_new(
    rage_ElementType * type, uint32_t sample_rate);
void rage_element_free(rage_Element * elem);
void rage_element_process(
    rage_Element const * const elem, rage_TransportState const transport_state,
    uint32_t period_size, rage_Ports const * ports);


// FIXME: better or worse?
#define RAGE_ELEM_PREP(elem, controls) elem->type->prep(elem->state, controls)
#define RAGE_ELEM_CLEAN(elem, controls) elem->type->clean(elem->state, controls)
#define RAGE_ELEM_CLEAR(elem, ...) elem->type->clear(elem->state, __VA_ARGS__)
