#pragma once
#include "macros.h"

typedef enum {
    RAGE_LIBERTY_MUST_PROVIDE,
    RAGE_LIBERTY_MAY_PROVIDE,
    RAGE_LIBERTY_CANNOT_PROVIDE
} rage_Liberty;

typedef struct {
    RAGE_MAYBE(int) min;
    RAGE_MAYBE(int) max;
} rage_IntConstraints;

typedef struct {
    RAGE_MAYBE(float) min;
    RAGE_MAYBE(float) max;
} rage_FloatConstraints;

typedef RAGE_MAYBE(char const *) rage_StringConstraints;

typedef enum {
    RAGE_ATOM_INT,
    RAGE_ATOM_FLOAT,
    RAGE_ATOM_STRING
} rage_AtomType;

typedef struct {
    rage_AtomType t;
    char * name;
    union {
        rage_IntConstraints i;
        rage_FloatConstraints f;
        rage_StringConstraints s;
    } constraints;
} rage_AtomDef;

typedef struct {
    char * name;
    char * description;
    rage_Liberty liberty;
    char ** fields;
    rage_AtomDef * types;
} rage_TupleDef;

typedef union {
    float f;
    int i;
    char * s;
} rage_Atom;


typedef RAGE_ARRAY(rage_Atom) rage_Tuple;
