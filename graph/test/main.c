#include "test_loader.c"
#include "test_engine.c"

int main() {
    rage_Error e = test();
    if (RAGE_FAILED(e)) {
        printf("Failed: %s\n", RAGE_FAILURE_VALUE(e));
        return 1;
    }
    e = test_engine();
    if (RAGE_FAILED(e)) {
        printf("Failed: %s\n", RAGE_FAILURE_VALUE(e));
        return 1;
    }
}
