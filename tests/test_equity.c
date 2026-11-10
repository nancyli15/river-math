#include "rivermath/equity.h"
#include "test.h"

/* Input for players given as strings ("" = random) and a board. */
static rm_equity_input make_input(int num_players, const char *const players[], const char *board) {
    rm_equity_input input = {.num_players = num_players, .board = test_cards(board), .seed = 1};
    for (int player = 0; player < num_players; player++)
        input.hole_cards[player] = test_cards(players[player]);
    return input;
}

static void test_board_plays_is_an_exact_split(void) {
    /* Both players' best hand is the board itself: A A K K Q. */
    const char *players[] = {"2c 3d", "4c 5d"};
    rm_equity_input input = make_input(2, players, "Ah Ad Kc Kd Qh");
    rm_equity_result result;
    CHECK_EQ(rm_equity(&input, &result), 0);
    CHECK(result.was_exact);
    CHECK_EQ(result.num_deals, 1);
    CHECK_NEAR(result.equity[0], 0.5, 1e-12);
    CHECK_NEAR(result.equity[1], 0.5, 1e-12);
    CHECK_NEAR(result.tie[0], 1.0, 1e-12);
}

static void test_river_has_one_winner(void) {
    const char *players[] = {"As Ac", "Kh Kc"};
    rm_equity_input input = make_input(2, players, "2c 7d 9h Js 3s");
    rm_equity_result result;
    CHECK_EQ(rm_equity(&input, &result), 0);
    CHECK_NEAR(result.equity[0], 1.0, 1e-12);
    CHECK_NEAR(result.equity[1], 0.0, 1e-12);
}

/*
 * Counted by hand: on the turn, QJ needs a ten for a straight (9-T-J-Q-K).
 * 52 - 4 board - 4 hole = 44 river cards, 4 of them tens, so QJ wins 4/44.
 */
static void test_turn_outs_counted_by_hand(void) {
    const char *players[] = {"As Ah", "Qh Jh"};
    rm_equity_input input = make_input(2, players, "2c 7d 9h Ks");
    rm_equity_result result;
    CHECK_EQ(rm_equity(&input, &result), 0);
    CHECK_EQ(result.num_deals, 44);
    CHECK_NEAR(result.equity[1], 4.0 / 44.0, 1e-12);
    CHECK_NEAR(result.equity[0], 40.0 / 44.0, 1e-12);
}

/* Well-known preflop matchup: aces beat kings about 82% of the time. */
static void test_aces_vs_kings_preflop(void) {
    const char *players[] = {"Ah As", "Kd Kc"};
    rm_equity_input input = make_input(2, players, "");
    rm_equity_result result;
    CHECK_EQ(rm_equity(&input, &result), 0);
    CHECK_EQ(result.num_deals, 1712304); /* C(48, 5) boards */
    CHECK_NEAR(result.equity[0], 0.82, 0.01);
    CHECK_NEAR(result.equity[0] + result.equity[1], 1.0, 1e-9);
}

/* 9 known hands preflop: C(34, 5) boards, and the pot always adds up to 1. */
static void test_nine_known_players(void) {
    const char *players[] = {"Ah As", "Kd Kc", "Qh Qs", "Jd Jc", "Th Ts",
                             "9d 9c", "8h 8s", "7d 7c", "6h 6s"};
    rm_equity_input input = make_input(9, players, "");
    rm_equity_result result;
    CHECK_EQ(rm_equity(&input, &result), 0);
    CHECK_EQ(result.num_deals, 278256);
    double total = 0;
    for (int player = 0; player < 9; player++) total += result.equity[player];
    CHECK_NEAR(total, 1.0, 1e-9);
    CHECK(result.equity[0] > result.equity[8]); /* aces beat sixes */
}

/* Sampling the same spot should land close to the exact answer. */
static void test_monte_carlo_matches_exact(void) {
    const char *players[] = {"Ah Kh", "Qs Qd", "7c 6c"};
    rm_equity_input input = make_input(3, players, "Qh 8c 5d");
    rm_equity_result exact, sampled;
    CHECK_EQ(rm_equity(&input, &exact), 0);
    CHECK(exact.was_exact);

    input.force_monte_carlo = true;
    input.num_samples = 200000;
    CHECK_EQ(rm_equity(&input, &sampled), 0);
    CHECK(!sampled.was_exact);
    CHECK_EQ(sampled.num_deals, 200000);
    /* Standard error is at most sqrt(0.25 / 200000) = 0.0011; allow about 4 of them. */
    for (int player = 0; player < 3; player++)
        CHECK_NEAR(sampled.equity[player], exact.equity[player], 0.005);
}

/* Known value: pocket aces win about 85% against one random hand. */
static void test_aces_vs_random_hand(void) {
    const char *players[] = {"Ah As", ""};
    rm_equity_input input = make_input(2, players, "");
    input.num_samples = 200000;
    rm_equity_result result;
    CHECK_EQ(rm_equity(&input, &result), 0);
    CHECK(!result.was_exact);
    CHECK_NEAR(result.equity[0], 0.852, 0.006);
}

static void test_same_seed_same_answer(void) {
    const char *players[] = {"Ah As", "", ""};
    rm_equity_input input = make_input(3, players, "");
    input.num_samples = 20000;
    rm_equity_result first, second;
    rm_equity(&input, &first);
    rm_equity(&input, &second);
    CHECK(first.equity[0] == second.equity[0]);
    input.seed = 2;
    rm_equity(&input, &second);
    CHECK(first.equity[0] != second.equity[0]);
}

static void test_dead_cards_are_not_dealt(void) {
    /* All four tens are dead, so QJ has no outs. */
    const char *players[] = {"As Ah", "Qh Jh"};
    rm_equity_input input = make_input(2, players, "2c 7d 9h Ks");
    input.dead_cards = test_cards("Tc Td Th Ts");
    rm_equity_result result;
    CHECK_EQ(rm_equity(&input, &result), 0);
    CHECK_EQ(result.num_deals, 40);
    CHECK_NEAR(result.equity[1], 0.0, 1e-12);
}

static void test_invalid_inputs(void) {
    rm_equity_result result;
    const char *two_players[] = {"Ah As", "Kd Kc"};

    rm_equity_input input = make_input(2, two_players, "");
    input.num_players = 1;
    CHECK_EQ(rm_equity(&input, &result), RM_ERR_NUM_PLAYERS);
    input.num_players = 10;
    CHECK_EQ(rm_equity(&input, &result), RM_ERR_NUM_PLAYERS);

    const char *one_card[] = {"Ah", "Kd Kc"};
    input = make_input(2, one_card, "");
    CHECK_EQ(rm_equity(&input, &result), RM_ERR_HOLE_CARDS);

    input = make_input(2, two_players, "2c 3c");
    CHECK_EQ(rm_equity(&input, &result), RM_ERR_BOARD);

    input = make_input(2, two_players, "Ah 3c 4c"); /* Ah is also in player 1's hand */
    CHECK_EQ(rm_equity(&input, &result), RM_ERR_CARD_REUSED);

    const char *same_card[] = {"Ah As", "Ah Kc"};
    input = make_input(2, same_card, "");
    CHECK_EQ(rm_equity(&input, &result), RM_ERR_CARD_REUSED);

    input = make_input(2, two_players, "");
    input.dead_cards = RM_FULL_DECK & ~(test_cards("Ah As Kd Kc 2c 3c 4c"));
    CHECK_EQ(rm_equity(&input, &result), RM_ERR_DECK_EMPTY); /* only 3 cards left for 5 board cards */
}

void suite_equity(void) {
    RUN_TEST(test_board_plays_is_an_exact_split);
    RUN_TEST(test_river_has_one_winner);
    RUN_TEST(test_turn_outs_counted_by_hand);
    RUN_TEST(test_aces_vs_kings_preflop);
    RUN_TEST(test_nine_known_players);
    RUN_TEST(test_monte_carlo_matches_exact);
    RUN_TEST(test_aces_vs_random_hand);
    RUN_TEST(test_same_seed_same_answer);
    RUN_TEST(test_dead_cards_are_not_dealt);
    RUN_TEST(test_invalid_inputs);
}
