#include "rivermath/card.h"
#include "test.h"

/* Every card maps to its own bit, and all 52 bits together are the full deck. */
static void test_each_card_has_unique_bit(void) {
    rm_cards all_cards = 0;
    for (int card = 0; card < RM_NUM_CARDS; card++) {
        rm_cards just_this_card = rm_card_set(card);
        CHECK_EQ(rm_count_cards(just_this_card), 1);
        CHECK((all_cards & just_this_card) == 0);
        all_cards |= just_this_card;
        CHECK_EQ(rm_bit_to_card(rm_card_to_bit(card)), card);
    }
    CHECK(all_cards == RM_FULL_DECK);
    CHECK_EQ(rm_count_cards(RM_FULL_DECK), 52);
    CHECK(rm_is_valid_card_set(RM_FULL_DECK));
    CHECK(!rm_is_valid_card_set(1ULL << 13)); /* unused bit in the clubs lane */
}

static void test_card_layout(void) {
    CHECK_EQ(rm_make_card(RM_RANK_2, RM_CLUBS), 0);
    CHECK_EQ(rm_make_card(RM_RANK_A, RM_SPADES), 51);
    int queen_of_hearts = rm_make_card(RM_RANK_Q, RM_HEARTS);
    CHECK_EQ(rm_card_rank(queen_of_hearts), RM_RANK_Q);
    CHECK_EQ(rm_card_suit(queen_of_hearts), RM_HEARTS);
    /* bit = suit * 16 + rank */
    CHECK_EQ(rm_card_to_bit(rm_make_card(RM_RANK_2, RM_CLUBS)), 0);
    CHECK_EQ(rm_card_to_bit(rm_make_card(RM_RANK_A, RM_SPADES)), 3 * 16 + 12);
}

/* Every card survives format -> parse. */
static void test_parse_format_roundtrip(void) {
    char text[3];
    for (int card = 0; card < RM_NUM_CARDS; card++)
        CHECK_EQ(rm_parse_card(rm_format_card(card, text)), card);
}

static void test_parse_card(void) {
    CHECK_EQ(rm_parse_card("Ah"), rm_make_card(RM_RANK_A, RM_HEARTS));
    CHECK_EQ(rm_parse_card("2c"), rm_make_card(RM_RANK_2, RM_CLUBS));
    CHECK_EQ(rm_parse_card("Ts"), rm_make_card(RM_RANK_T, RM_SPADES));
    /* case-insensitive */
    CHECK_EQ(rm_parse_card("ah"), rm_make_card(RM_RANK_A, RM_HEARTS));
    CHECK_EQ(rm_parse_card("tD"), rm_make_card(RM_RANK_T, RM_DIAMONDS));
    /* invalid */
    CHECK_EQ(rm_parse_card(""), RM_ERR_SYNTAX);
    CHECK_EQ(rm_parse_card("A"), RM_ERR_SYNTAX);
    CHECK_EQ(rm_parse_card("Ax"), RM_ERR_SYNTAX);
    CHECK_EQ(rm_parse_card("1h"), RM_ERR_SYNTAX);
    CHECK_EQ(rm_parse_card("hA"), RM_ERR_SYNTAX);
    CHECK_EQ(rm_parse_card(NULL), RM_ERR_SYNTAX);
}

static void test_parse_cards(void) {
    rm_cards cards = 0;
    CHECK_EQ(rm_parse_cards("AhKd", &cards), 2);
    CHECK(cards == (rm_card_set(rm_parse_card("Ah")) | rm_card_set(rm_parse_card("Kd"))));

    CHECK_EQ(rm_parse_cards("Ah Kd, 2c  ", &cards), 3);
    CHECK_EQ(rm_count_cards(cards), 3);

    CHECK_EQ(rm_parse_cards("", &cards), 0);
    CHECK(cards == 0);

    /* errors leave the output untouched */
    cards = 123;
    CHECK_EQ(rm_parse_cards("AhAh", &cards), RM_ERR_DUPLICATE);
    CHECK_EQ(rm_parse_cards("Ah ah", &cards), RM_ERR_DUPLICATE);
    CHECK_EQ(rm_parse_cards("AhK", &cards), RM_ERR_SYNTAX);
    CHECK_EQ(rm_parse_cards("AhXx", &cards), RM_ERR_SYNTAX);
    CHECK_EQ(rm_parse_cards("10h", &cards), RM_ERR_SYNTAX);
    CHECK(cards == 123);
}

static void test_format_cards(void) {
    rm_cards cards;
    char text[128];

    rm_parse_cards("2c Kd Ah Tc Ad", &cards);
    CHECK_EQ(rm_format_cards(cards, text, sizeof text), 10);
    CHECK_STR(text, "AdAhKdTc2c"); /* highest rank first, then suits c d h s */

    CHECK_EQ(rm_format_cards(0, text, sizeof text), 0);
    CHECK_STR(text, "");

    CHECK_EQ(rm_format_cards(RM_FULL_DECK, text, sizeof text), 104);
    CHECK_EQ(strlen(text), 104);

    /* a too-small buffer is truncated like snprintf */
    rm_parse_cards("AhKd", &cards);
    CHECK_EQ(rm_format_cards(cards, text, 4), 4);
    CHECK_STR(text, "AhK");
    CHECK_EQ(rm_format_cards(cards, text, 1), 4);
    CHECK_STR(text, "");
}

static void test_ranks_in_suit(void) {
    rm_cards cards;
    rm_parse_cards("Ah Kh 2h 3c Ts", &cards);
    CHECK_EQ(rm_ranks_in_suit(cards, RM_HEARTS),
             (1u << RM_RANK_A) | (1u << RM_RANK_K) | (1u << RM_RANK_2));
    CHECK_EQ(rm_ranks_in_suit(cards, RM_CLUBS), 1u << RM_RANK_3);
    CHECK_EQ(rm_ranks_in_suit(cards, RM_SPADES), 1u << RM_RANK_T);
    CHECK_EQ(rm_ranks_in_suit(cards, RM_DIAMONDS), 0);
}

static void test_pop_card(void) {
    rm_cards original;
    rm_parse_cards("As 2c Kh", &original);

    rm_cards remaining = original;
    rm_cards rebuilt = 0;
    int num_popped = 0;
    int card;
    while ((card = rm_pop_card(&remaining)) >= 0) {
        rebuilt |= rm_card_set(card);
        num_popped++;
    }
    CHECK_EQ(num_popped, 3);
    CHECK(rebuilt == original);
    CHECK(remaining == 0);
    CHECK_EQ(rm_pop_card(&remaining), -1);
}

void suite_card(void) {
    RUN_TEST(test_each_card_has_unique_bit);
    RUN_TEST(test_card_layout);
    RUN_TEST(test_parse_format_roundtrip);
    RUN_TEST(test_parse_card);
    RUN_TEST(test_parse_cards);
    RUN_TEST(test_format_cards);
    RUN_TEST(test_ranks_in_suit);
    RUN_TEST(test_pop_card);
}
