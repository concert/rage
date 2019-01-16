#pragma once

typedef void (*rage_hs_Deallocator)(void *);
typedef struct rage_hs_RefCount rage_hs_RefCount;

rage_hs_RefCount * rage_hs_count_ref(rage_hs_Deallocator dealloc, void * ref);
#define RAGE_HS_COUNT_REF(f, p) rage_hs_count_ref((rage_hs_Deallocator) f, p)

void rage_hs_depend_ref(rage_hs_RefCount * depender, rage_hs_RefCount * dependee);
void rage_hs_decrement_ref(rage_hs_RefCount * ref);

void * rage_hs_ref(rage_hs_RefCount const * rc);
