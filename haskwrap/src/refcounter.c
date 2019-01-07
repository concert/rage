#include <stdlib.h>
#include "refcounter.h"

typedef struct rage_hs_RefCountList rage_hs_RefCountList;

struct rage_hs_RefCount {
    unsigned count;
    void * ref;
    rage_hs_Deallocator dealloc;
    rage_hs_RefCountList * reqs;
};

struct rage_hs_RefCountList {
    rage_hs_RefCountList * next;
    rage_hs_RefCount * rc;
};

rage_hs_RefCount * rage_hs_count_ref(rage_hs_Deallocator dealloc, void * ref) {
    rage_hs_RefCount * new_ref = malloc(sizeof(rage_hs_RefCount));
    new_ref->count = 1;
    new_ref->ref = ref;
    new_ref->dealloc = dealloc;
    new_ref->reqs = NULL;
    return new_ref;
}

void rage_hs_depend_ref(rage_hs_RefCount * depender, rage_hs_RefCount * dependee) {
    dependee->count++;
    rage_hs_RefCountList * newl = malloc(sizeof(rage_hs_RefCountList));
    newl->next = depender->reqs;
    depender->reqs = newl;
    newl->rc = dependee;
}

void rage_hs_decrement_ref(rage_hs_RefCount * ref) {
    if (--ref->count == 0) {
        ref->dealloc(ref->ref);
        while (ref->reqs) {
            rage_hs_decrement_ref(ref->reqs->rc);
            rage_hs_RefCountList * next = ref->reqs->next;
            free(ref->reqs);
            ref->reqs = next;
        }
        free(ref);
    }
}

void * rage_hs_ref(rage_hs_RefCount const * rc) {
    return rc->ref;
}
