#pragma once
#include "time.h"
#include "macros.h"

typedef enum {
    RAGE_LIBERTY_CANNOT_PROVIDE,
    RAGE_LIBERTY_MAY_PROVIDE,
    RAGE_LIBERTY_MUST_PROVIDE
} rage_Liberty;

#define RAGE_MINMAX(type) \
    struct { \
        RAGE_MAYBE(type) min; \
        RAGE_MAYBE(type) max; \
    }

typedef RAGE_MINMAX(int) rage_IntConstraints;
typedef RAGE_MINMAX(float) rage_FloatConstraints;
typedef RAGE_MINMAX(rage_Time) rage_TimeConstraints;

#undef RAGE_MINMAX

typedef RAGE_MAYBE(char const *) rage_StringConstraints;

typedef struct {
    int value;
    char * name;
} rage_EnumOpt;

typedef RAGE_ARRAY(rage_EnumOpt const) rage_EnumConstraints;

typedef enum {
    RAGE_ATOM_INT,
    RAGE_ATOM_FLOAT,
    RAGE_ATOM_TIME,
    RAGE_ATOM_STRING,
    RAGE_ATOM_ENUM
} rage_AtomType;

typedef struct {
    rage_AtomType type;
    char const * name;
    union {
        rage_IntConstraints i;
        rage_FloatConstraints f;
        rage_TimeConstraints t;
        rage_StringConstraints s;
        rage_EnumConstraints e;
    } constraints;
} rage_AtomDef;

typedef struct {
    char const * name;
    rage_AtomDef const * type;
} rage_FieldDef;

typedef struct {
    char * name;
    char * description;
    rage_Liberty liberty;
    RAGE_ARRAY(rage_FieldDef const);
} rage_TupleDef;

typedef union {
    int i;
    float f;
    rage_Time t;
    char * s;
    int e;
} rage_Atom;

rage_Atom * rage_tuple_generate(rage_TupleDef const * const td);
rage_Atom * rage_tuple_copy(rage_TupleDef const * const td, rage_Atom const * const tup);
void rage_tuple_free(rage_TupleDef const * const td, rage_Atom * const tup);
