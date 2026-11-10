#include "rivermath/random.h"
#include "test.h"

static void test_same_seed_same_numbers(void) {
    rm_random first, second;
    rm_random_seed(&first, 42);
    rm_random_seed(&second, 42);
    for (int i = 0; i < 100; i++) CHECK(rm_random_next(&first) == rm_random_next(&second));

    rm_random_seed(&second, 43);
    CHECK(rm_random_next(&first) != rm_random_next(&second));
}

static void test_random_below_stays_in_range(void) {
    rm_random random;
    rm_random_seed(&random, 1);
    bool in_range = true;
    for (int i = 0; i < 10000; i++)
        if (rm_random_below(&random, 7) >= 7) in_range = false;
    CHECK(in_range);
    CHECK_EQ(rm_random_below(&random, 1), 0);
}

static void test_deal_gives_distinct_available_cards(void) {
    rm_random random;
    rm_random_seed(&random, 7);
    rm_cards available;
    rm_parse_cards("Ah Kd Qc Js Th 9d 8c", &available);
    for (int i = 0; i < 1000; i++) {
        rm_cards dealt = rm_deal_random_cards(&random, available, 3);
        CHECK_EQ(rm_count_cards(dealt), 3);
        CHECK((dealt & ~available) == 0);
    }
    CHECK(rm_deal_random_cards(&random, available, 7) == available);
}

/* Each of the 52 cards should come up about equally often. */
static void test_deal_is_roughly_uniform(void) {
    rm_random random;
    rm_random_seed(&random, 99);
    int times_dealt[RM_NUM_CARDS] = {0};
    const int num_deals = 52000;
    for (int i = 0; i < num_deals; i++) {
        rm_cards dealt = rm_deal_random_cards(&random, RM_FULL_DECK, 1);
        times_dealt[rm_pop_card(&dealt)]++;
    }
    /* Expected 1000 each; a fair deck stays well within +-15% at this sample size. */
    int num_outliers = 0;
    for (int card = 0; card < RM_NUM_CARDS; card++)
        if (times_dealt[card] < 850 || times_dealt[card] > 1150) num_outliers++;
    CHECK_EQ(num_outliers, 0);
}

void suite_random(void) {
    RUN_TEST(test_same_seed_same_numbers);
    RUN_TEST(test_random_below_stays_in_range);
    RUN_TEST(test_deal_gives_distinct_available_cards);
    RUN_TEST(test_deal_is_roughly_uniform);
}
