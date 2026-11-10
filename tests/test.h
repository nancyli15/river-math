#ifndef RIVERMATH_TEST_H
#define RIVERMATH_TEST_H

/*
 * Minimal test harness.
 *
 *   static void test_something(void) {
 *       CHECK(x > 0);
 *       CHECK_EQ(rm_parse_card("Ah"), 50);
 *       CHECK_STR(buf, "AhKd");
 *   }
 *
 * Register suites in test_main.c. A failed check prints file:line and keeps
 * going, so one run reports every failure.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

extern int rm_test_checks;
extern int rm_test_failures;

#define CHECK(cond)                                                        \
    do {                                                                   \
        rm_test_checks++;                                                  \
        if (!(cond)) {                                                     \
            rm_test_failures++;                                            \
            fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                  \
    } while (0)

#define CHECK_EQ(actual, expected)                                         \
    do {                                                                   \
        long long actual_ = (long long)(actual), expected_ = (long long)(expected);    \
        rm_test_checks++;                                                  \
        if (actual_ != expected_) {                                                    \
            rm_test_failures++;                                            \
            fprintf(stderr, "  FAIL %s:%d: %s == %lld, expected %lld\n",   \
                    __FILE__, __LINE__, #actual, actual_, expected_);                  \
        }                                                                  \
    } while (0)

#define CHECK_STR(actual, expected)                                        \
    do {                                                                   \
        const char *actual_ = (actual), *expected_ = (expected);                       \
        rm_test_checks++;                                                  \
        if (strcmp(actual_, expected_) != 0) {                                         \
            rm_test_failures++;                                            \
            fprintf(stderr, "  FAIL %s:%d: %s == \"%s\", expected \"%s\"\n", \
                    __FILE__, __LINE__, #actual, actual_, expected_);                  \
        }                                                                  \
    } while (0)

#define RUN_TEST(fn)                                                       \
    do {                                                                   \
        int failures_before_ = rm_test_failures;                                    \
        fn();                                                              \
        printf("  %-40s %s\n", #fn, rm_test_failures == failures_before_ ? "ok" : "FAILED"); \
    } while (0)

/* Suites (one per test_*.c file). */
void suite_bits(void);
void suite_card(void);
void suite_random(void);
void suite_evaluate(void);
void suite_evaluate_full(void); /* slow: every 7-card hand */

#endif /* RIVERMATH_TEST_H */
