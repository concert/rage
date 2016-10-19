#include <stdio.h>
#include "loader.h"

void test() {
    rage_ElementLoader el = rage_element_loader_new();
    rage_ElementTypes element_type_names = rage_element_loader_list(el);
    for (unsigned i=0; i<element_type_names.len; i++) {
        printf("%s\n", element_type_names.items[i]);
    }
    rage_element_loader_free(el);
}

int main() {
    test();
}
