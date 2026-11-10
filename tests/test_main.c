#include "test.h"

int rm_test_checks = 0;
int rm_test_failures = 0;

int main(void) {
    printf("bits\n");
    suite_bits();
    printf("card\n");
    suite_card();

    printf("\n%d checks, %d failures\n", rm_test_checks, rm_test_failures);
    return rm_test_failures == 0 ? 0 : 1;
}
