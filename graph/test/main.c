#include "test_loader.c"
#include "test_engine.c"

typedef rage_Error (*rage_TestFunc)();

int main() {
    unsigned const len = 2;
    rage_TestFunc tests[] = {
        test,
        test_engine
    };
    for (unsigned i=0; i < len; i++) {
        rage_Error e = tests[i]();
        if (RAGE_FAILED(e)) {
            printf("Failed: %s\n", RAGE_FAILURE_VALUE(e));
            return 1;
        }
    }
}
