#include "rivermath/equity.h"

#include <string.h>

#include "rivermath/evaluate.h"
#include "rivermath/random.h"

enum { BOARD_SIZE = 5, HOLE_SIZE = 2 };

/* Running totals while dealing. */
typedef struct {
    int num_players;
    double pot_share[RM_MAX_PLAYERS];
    uint64_t wins[RM_MAX_PLAYERS];
    uint64_t ties[RM_MAX_PLAYERS];
    uint64_t num_deals;
} tally;

/* Score one finished deal: find the best hand and split this pot among the winners. */
static void score_one_deal(tally *totals, const rm_cards hole_cards[], rm_cards board) {
    rm_hand_value values[RM_MAX_PLAYERS];
    rm_hand_value best = 0;
    for (int player = 0; player < totals->num_players; player++) {
        values[player] = rm_evaluate_lookup(hole_cards[player] | board);
        if (values[player] > best) best = values[player];
    }

    int num_winners = 0;
    for (int player = 0; player < totals->num_players; player++)
        if (values[player] == best) num_winners++;

    for (int player = 0; player < totals->num_players; player++) {
        if (values[player] != best) continue;
        totals->pot_share[player] += 1.0 / num_winners;
        if (num_winners == 1) totals->wins[player]++;
        else totals->ties[player]++;
    }
    totals->num_deals++;
}

/* ---- exact: every possible finished board ------------------------------------ */

typedef struct {
    tally *totals;
    const rm_cards *hole_cards;
    int deck[RM_NUM_CARDS]; /* cards not used anywhere yet */
    int deck_size;
} board_enumeration;

/* Add cards_left more cards from deck[first..] to the board, in increasing order so each board is seen once. */
static void enumerate_boards(board_enumeration *state, rm_cards board, int first, int cards_left) {
    if (cards_left == 0) {
        score_one_deal(state->totals, state->hole_cards, board);
        return;
    }
    for (int i = first; i <= state->deck_size - cards_left; i++)
        enumerate_boards(state, board | rm_card_set(state->deck[i]), i + 1, cards_left - 1);
}

static void run_exact(tally *totals, const rm_equity_input *input, rm_cards used_cards) {
    board_enumeration state = {.totals = totals, .hole_cards = input->hole_cards};
    rm_cards unused = RM_FULL_DECK & ~used_cards;
    int card;
    while ((card = rm_pop_card(&unused)) >= 0) state.deck[state.deck_size++] = card;

    int missing_board_cards = BOARD_SIZE - rm_count_cards(input->board);
    enumerate_boards(&state, input->board, 0, missing_board_cards);
}

/* ---- Monte Carlo: random hands and boards ------------------------------------ */

static void run_monte_carlo(tally *totals, const rm_equity_input *input, rm_cards used_cards) {
    rm_random random;
    rm_random_seed(&random, input->seed);
    uint64_t num_samples = input->num_samples ? input->num_samples : RM_DEFAULT_SAMPLES;
    int missing_board_cards = BOARD_SIZE - rm_count_cards(input->board);

    rm_cards hole_cards[RM_MAX_PLAYERS];
    for (uint64_t sample = 0; sample < num_samples; sample++) {
        rm_cards deck = RM_FULL_DECK & ~used_cards;
        /* Deal each random player separately so who gets which cards is random too. */
        for (int player = 0; player < input->num_players; player++) {
            hole_cards[player] = input->hole_cards[player];
            if (hole_cards[player] == 0) {
                hole_cards[player] = rm_deal_random_cards(&random, deck, HOLE_SIZE);
                deck &= ~hole_cards[player];
            }
        }
        rm_cards board = input->board | rm_deal_random_cards(&random, deck, missing_board_cards);
        score_one_deal(totals, hole_cards, board);
    }
}

/* ---- entry point ---------------------------------------------------------------- */

/* Check the input; on success *used_cards holds every known card. */
static int validate_input(const rm_equity_input *input, rm_cards *used_cards, bool *all_known) {
    if (input->num_players < 2 || input->num_players > RM_MAX_PLAYERS) return RM_ERR_NUM_PLAYERS;

    int board_size = rm_count_cards(input->board);
    if (board_size != 0 && board_size != 3 && board_size != 4 && board_size != 5)
        return RM_ERR_BOARD;

    rm_cards used = input->board;
    int num_known_cards = board_size;
    int cards_to_deal = BOARD_SIZE - board_size;
    for (int player = 0; player < input->num_players; player++) {
        int hole_size = rm_count_cards(input->hole_cards[player]);
        if (hole_size == 0) cards_to_deal += HOLE_SIZE;
        else if (hole_size != HOLE_SIZE) return RM_ERR_HOLE_CARDS;
        used |= input->hole_cards[player];
        num_known_cards += hole_size;
    }
    used |= input->dead_cards;
    num_known_cards += rm_count_cards(input->dead_cards);
    /* If a card was listed twice, the union has fewer cards than the total. */
    if (rm_count_cards(used) != num_known_cards || !rm_is_valid_card_set(used))
        return RM_ERR_CARD_REUSED;
    if (RM_NUM_CARDS - num_known_cards < cards_to_deal) return RM_ERR_DECK_EMPTY;

    *all_known = cards_to_deal == BOARD_SIZE - board_size; /* nothing to deal but the board */
    *used_cards = used;
    return 0;
}

int rm_equity(const rm_equity_input *input, rm_equity_result *result) {
    rm_cards used_cards;
    bool all_known;
    int error = validate_input(input, &used_cards, &all_known);
    if (error) return error;

    tally totals = {.num_players = input->num_players};
    bool exact = all_known && !input->force_monte_carlo;
    if (exact) run_exact(&totals, input, used_cards);
    else run_monte_carlo(&totals, input, used_cards);

    memset(result, 0, sizeof *result);
    result->num_deals = totals.num_deals;
    result->was_exact = exact;
    for (int player = 0; player < input->num_players; player++) {
        result->equity[player] = totals.pot_share[player] / (double)totals.num_deals;
        result->win[player] = (double)totals.wins[player] / (double)totals.num_deals;
        result->tie[player] = (double)totals.ties[player] / (double)totals.num_deals;
    }
    return 0;
}

const char *rm_equity_error_message(int error) {
    switch (error) {
    case RM_ERR_NUM_PLAYERS: return "need 2 to 9 players";
    case RM_ERR_HOLE_CARDS: return "each player needs exactly 2 cards (or \"random\")";
    case RM_ERR_BOARD: return "the board must have 0, 3, 4 or 5 cards";
    case RM_ERR_CARD_REUSED: return "a card is used more than once";
    case RM_ERR_DECK_EMPTY: return "not enough cards left in the deck";
    default: return "unknown error";
    }
}
