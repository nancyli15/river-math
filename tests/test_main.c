#include <stdlib.h>

#include "test.h"

int rm_test_checks = 0;
int rm_test_failures = 0;

rm_cards test_cards(const char *text) {
    rm_cards cards = 0;
    if (rm_parse_cards(text, &cards) < 0) {
        fprintf(stderr, "bad test cards: %s\n", text);
        abort();
    }
    return cards;
}

/* Pass --full to also run the slow exhaustive suites (make test-full). */
int main(int argc, char **argv) {
    bool run_full = argc > 1 && strcmp(argv[1], "--full") == 0;

    printf("bits\n");
    suite_bits();
    printf("card\n");
    suite_card();
    printf("random\n");
    suite_random();
    printf("evaluate\n");
    suite_evaluate();
    printf("equity\n");
    suite_equity();
    if (run_full) {
        printf("evaluate (full)\n");
        suite_evaluate_full();
    }

    printf("\n%d checks, %d failures\n", rm_test_checks, rm_test_failures);
    return rm_test_failures == 0 ? 0 : 1;
}
