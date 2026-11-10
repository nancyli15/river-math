#include <stdlib.h>

#include "rivermath/evaluate.h"
#include "rivermath/random.h"
#include "test.h"

typedef rm_hand_value (*evaluator_fn)(rm_cards hand);

typedef struct {
    const char *name;
    evaluator_fn evaluate;
} named_evaluator;

static const named_evaluator EVALUATORS[] = {
    {"naive", rm_evaluate_naive},
    {"bitwise", rm_evaluate_bitwise},
    {"lookup", rm_evaluate_lookup},
};
enum { NUM_EVALUATORS = sizeof EVALUATORS / sizeof EVALUATORS[0] };

static rm_cards cards_from(const char *text) {
    rm_cards cards = 0;
    if (rm_parse_cards(text, &cards) < 0) {
        fprintf(stderr, "bad test cards: %s\n", text);
        abort();
    }
    return cards;
}

/* Describe the hand with every evaluator and compare with the expected text. */
static void check_hand(const char *cards_text, const char *expected_description) {
    char description[64];
    for (int i = 0; i < NUM_EVALUATORS; i++) {
        rm_describe_hand_value(EVALUATORS[i].evaluate(cards_from(cards_text)), description,
                               sizeof description);
        if (strcmp(description, expected_description) != 0)
            fprintf(stderr, "  %s evaluator, hand %s:\n", EVALUATORS[i].name, cards_text);
        CHECK_STR(description, expected_description);
    }
}

/* -1 if hand a loses to hand b, 0 if they tie, 1 if a wins. Same answer from every evaluator. */
static int compare_hands(const char *hand_a, const char *hand_b) {
    int results[NUM_EVALUATORS];
    for (int i = 0; i < NUM_EVALUATORS; i++) {
        rm_hand_value a = EVALUATORS[i].evaluate(cards_from(hand_a));
        rm_hand_value b = EVALUATORS[i].evaluate(cards_from(hand_b));
        results[i] = (a > b) - (a < b);
        CHECK_EQ(results[i], results[0]);
    }
    return results[0];
}

static void test_straight_high_rank(void) {
    CHECK_EQ(rm_straight_high_rank(0), -1);
    CHECK_EQ(rm_straight_high_rank(0x1F), RM_RANK_6);              /* 2-6 */
    CHECK_EQ(rm_straight_high_rank(0x1F00), RM_RANK_A);            /* T-A */
    CHECK_EQ(rm_straight_high_rank((1u << RM_RANK_A) | 0xF), RM_RANK_5); /* wheel */
    CHECK_EQ(rm_straight_high_rank(0x7F), RM_RANK_8);              /* 2-8: best is 4-8 */
    CHECK_EQ(rm_straight_high_rank(0x1C03), -1);                   /* Q-K-A-2-3: no wrap-around */
}

static void test_every_category(void) {
    check_hand("9h 8h 7h 6h 5h", "Straight flush (9)");
    check_hand("Kc Kd Kh Ks 3d", "Four of a kind (K, 3)");
    check_hand("Qs Qh Qd 4c 4s", "Full house (Q, 4)");
    check_hand("Ad Jd 9d 6d 2d", "Flush (A, J, 9, 6, 2)");
    check_hand("Tc 9d 8h 7s 6c", "Straight (T)");
    check_hand("7c 7d 7h Ks 2d", "Three of a kind (7, K, 2)");
    check_hand("Jc Jd 5h 5s Ac", "Two pair (J, 5, A)");
    check_hand("Tc Th Ad 8s 4c", "One pair (T, A, 8, 4)");
    check_hand("Ac Jd 8h 6s 3c", "High card (A, J, 8, 6, 3)");
}

/* The edge cases from CONCEPTS.md Part 2. */
static void test_edge_cases(void) {
    check_hand("Ah 2c 3d 4h 5s", "Straight (5)");                  /* wheel: ace plays low */
    check_hand("Ah 2h 3h 4h 5h", "Straight flush (5)");
    check_hand("Qh Kd Ac 2s 3h", "High card (A, K, Q, 3, 2)");    /* no wrap-around */
    check_hand("Ah Kd Qc Js Th", "Straight (A)");
    check_hand("4c 5d 6h 7s 8c 9d 2h", "Straight (9)");          /* 6 in a row: top 5 */
    check_hand("Kc Kd 9h 9s 4c 4d Ah", "Two pair (K, 9, A)");     /* three pairs */
    check_hand("Kc Kd 9h 9s 4c 4d 2h", "Two pair (K, 9, 4)");     /* third pair is the kicker */
    check_hand("Kc Kd Kh 9c 9d 9h 2s", "Full house (K, 9)");      /* two sets of three */
    check_hand("7c 7d 7h 5c 5d Qc Qd", "Full house (7, Q)");      /* best pair, not first */
    check_hand("Ah Kh Qh 9h 3h 2h 5c", "Flush (A, K, Q, 9, 3)");  /* 6-card flush */
    check_hand("9h 8h 7c 6h 5d 2h Kh", "Flush (K, 9, 8, 6, 2)");  /* straight + flush != SF */
    check_hand("Kc Kd Kh Ks 2c 3d Ah", "Four of a kind (K, A)");  /* best kicker */
    check_hand("Kc Kd Kh Ks Qc Qd Qh", "Four of a kind (K, Q)");
    check_hand("2c 2d Ah Ad Kc Kd Qh", "Two pair (A, K, Q)");     /* counterfeited 22 */
    check_hand("9s 8s 7s 6s 5s 4s 3s", "Straight flush (9)");
}

static void test_comparisons(void) {
    /* every category beats the one below it */
    const char *by_strength[] = {
        "Ac Jd 8h 6s 3c", "Tc Th Ad 8s 4c", "Jc Jd 5h 5s Ac", "7c 7d 7h Ks 2d", "Tc 9d 8h 7s 6c",
        "Ad Jd 9d 6d 2d", "Qs Qh Qd 4c 4s", "Kc Kd Kh Ks 3d", "9h 8h 7h 6h 5h",
    };
    for (int i = 0; i + 1 < (int)(sizeof by_strength / sizeof by_strength[0]); i++)
        CHECK_EQ(compare_hands(by_strength[i], by_strength[i + 1]), -1);

    CHECK_EQ(compare_hands("Ah 2c 3d 4h 5s", "2c 3d 4h 5s 6c"), -1); /* wheel is the lowest straight */
    CHECK_EQ(compare_hands("Ac Ad Kh 7s 2c", "Ah As Kd 7c 2d"), 0);  /* suits never break ties */
    CHECK_EQ(compare_hands("Ac Ad Kh 7s 3c", "Ah As Kd 7c 2d"), 1);  /* last kicker decides */
    /* both players play the board: hole cards 22 vs 34 are worthless here */
    CHECK_EQ(compare_hands("2c 2d Ah Ad Kc Kd Qh", "3c 4d Ah Ad Kc Kd Qh"), 0);
}

static void test_describe(void) {
    char text[8];
    int full_length = rm_describe_hand_value(rm_evaluate_bitwise(cards_from("Qs Qh Qd 4c 4s")),
                                             text, sizeof text);
    CHECK_EQ(full_length, (int)strlen("Full house (Q, 4)"));
    CHECK_STR(text, "Full ho"); /* truncated like snprintf */
}

/* ---- exhaustive tests ------------------------------------------------------ */

/* Category counts from PLAN.md §4. */
static const long long FIVE_CARD_COUNTS[RM_NUM_CATEGORIES] = {
    1302540, 1098240, 123552, 54912, 10200, 5108, 3744, 624, 40,
};
static const long long SEVEN_CARD_COUNTS[RM_NUM_CATEGORIES] = {
    23294460, 58627800, 31433400, 6461620, 6180020, 4047644, 3473184, 224848, 41584,
};

typedef struct {
    long long category_counts[RM_NUM_CATEGORIES];
    unsigned char *value_seen; /* one flag per possible hand value */
    long long num_distinct_values;
    long long num_disagreements;
    evaluator_fn evaluate;
    evaluator_fn compare_with; /* optional second evaluator that must agree */
} enumeration_stats;

static void record_hand(enumeration_stats *stats, rm_cards hand) {
    rm_hand_value value = stats->evaluate(hand);
    stats->category_counts[rm_hand_category(value)]++;
    if (!stats->value_seen[value]) {
        stats->value_seen[value] = 1;
        stats->num_distinct_values++;
    }
    if (stats->compare_with && stats->compare_with(hand) != value) {
        if (stats->num_disagreements++ < 5) {
            char text[16];
            rm_format_cards(hand, text, sizeof text);
            fprintf(stderr, "  evaluators disagree on %s\n", text);
        }
    }
}

/* Visit every hand of cards_left more cards chosen from cards first_card..51. */
static void enumerate_hands(enumeration_stats *stats, rm_cards hand, int first_card,
                            int cards_left) {
    if (cards_left == 0) {
        record_hand(stats, hand);
        return;
    }
    for (int card = first_card; card <= RM_NUM_CARDS - cards_left; card++)
        enumerate_hands(stats, hand | rm_card_set(card), card + 1, cards_left - 1);
}

static void check_all_hands(int hand_size, evaluator_fn evaluate, evaluator_fn compare_with,
                            const long long expected_counts[], long long expected_distinct) {
    enumeration_stats stats = {0};
    stats.evaluate = evaluate;
    stats.compare_with = compare_with;
    stats.value_seen = calloc(RM_HAND_VALUE_LIMIT, 1);
    enumerate_hands(&stats, 0, 0, hand_size);
    for (int category = 0; category < RM_NUM_CATEGORIES; category++)
        CHECK_EQ(stats.category_counts[category], expected_counts[category]);
    CHECK_EQ(stats.num_distinct_values, expected_distinct);
    CHECK_EQ(stats.num_disagreements, 0);
    free(stats.value_seen);
}

static void test_all_five_card_hands_naive(void) {
    check_all_hands(5, rm_evaluate_naive, NULL, FIVE_CARD_COUNTS, 7462);
}

static void test_all_five_card_hands_bitwise(void) {
    check_all_hands(5, rm_evaluate_bitwise, rm_evaluate_naive, FIVE_CARD_COUNTS, 7462);
}

/* A fast sample for `make test`; the full 133M-hand check is in suite_evaluate_full. */
static void test_random_seven_card_hands_all_agree(void) {
    rm_random random;
    rm_random_seed(&random, 2026);
    int num_disagreements = 0;
    for (int i = 0; i < 200000; i++) {
        rm_cards hand = rm_deal_random_cards(&random, RM_FULL_DECK, 7);
        rm_hand_value expected = rm_evaluate_naive(hand);
        if (rm_evaluate_bitwise(hand) != expected) num_disagreements++;
        if (rm_evaluate_lookup(hand) != expected) num_disagreements++;
    }
    CHECK_EQ(num_disagreements, 0);
}

static void test_lookup_tables_are_small(void) {
    /* A table indexed directly by key would be about 31 MB. */
    CHECK(rm_lookup_table_bytes() < 1024 * 1024);
}

static void test_all_seven_card_hands_bitwise_vs_naive(void) {
    check_all_hands(7, rm_evaluate_bitwise, rm_evaluate_naive, SEVEN_CARD_COUNTS, 4824);
}

static void test_all_seven_card_hands_lookup_vs_bitwise(void) {
    check_all_hands(7, rm_evaluate_lookup, rm_evaluate_bitwise, SEVEN_CARD_COUNTS, 4824);
}

void suite_evaluate(void) {
    RUN_TEST(test_straight_high_rank);
    RUN_TEST(test_every_category);
    RUN_TEST(test_edge_cases);
    RUN_TEST(test_comparisons);
    RUN_TEST(test_describe);
    RUN_TEST(test_all_five_card_hands_naive);
    RUN_TEST(test_all_five_card_hands_bitwise);
    RUN_TEST(test_random_seven_card_hands_all_agree);
    RUN_TEST(test_lookup_tables_are_small);
}

void suite_evaluate_full(void) {
    RUN_TEST(test_all_seven_card_hands_bitwise_vs_naive);
    RUN_TEST(test_all_seven_card_hands_lookup_vs_bitwise);
}
