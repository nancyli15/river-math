#ifndef RIVERMATH_EQUITY_H
#define RIVERMATH_EQUITY_H

/*
 * Equity: each player's share of the pot, averaged over every way the rest
 * of the board could come out. A k-way tie gives each tied player 1/k.
 *
 * - If every player's hole cards are known, every possible board is
 *   enumerated, so the answer is exact.
 * - If any player is "random" (hole cards = 0), there are too many
 *   possibilities to enumerate, so boards and random hands are sampled
 *   instead (Monte Carlo).
 */

#include <stdbool.h>
#include <stdint.h>

#include "rivermath/card.h"

enum {
    RM_MAX_PLAYERS = 9,
    RM_DEFAULT_SAMPLES = 1000000,
};

/* Error codes from rm_equity (negative; card.h uses -1 and -2). */
enum {
    RM_ERR_NUM_PLAYERS = -3, /* need 2 to 9 players */
    RM_ERR_HOLE_CARDS = -4,  /* each player needs exactly 2 cards, or 0 for random */
    RM_ERR_BOARD = -5,       /* board must have 0, 3, 4 or 5 cards */
    RM_ERR_CARD_REUSED = -6, /* a card appears in two places */
    RM_ERR_DECK_EMPTY = -7,  /* not enough cards left to deal */
};

typedef struct {
    int num_players;                     /* 2..9 */
    rm_cards hole_cards[RM_MAX_PLAYERS]; /* 0 = random player */
    rm_cards board;                      /* 0, 3, 4 or 5 cards */
    rm_cards dead_cards;                 /* cards known to be out of the deck */
    uint64_t num_samples;                /* Monte Carlo only; 0 = RM_DEFAULT_SAMPLES */
    uint64_t seed;                       /* Monte Carlo only */
    bool force_monte_carlo;              /* sample even when exact is possible (for testing) */
} rm_equity_input;

typedef struct {
    double equity[RM_MAX_PLAYERS]; /* 0..1, ties shared */
    double win[RM_MAX_PLAYERS];    /* fraction of deals won outright */
    double tie[RM_MAX_PLAYERS];    /* fraction of deals tied for best */
    uint64_t num_deals;            /* boards enumerated, or samples taken */
    bool was_exact;
} rm_equity_result;

/* Returns 0 on success or one of the negative error codes above. */
int rm_equity(const rm_equity_input *input, rm_equity_result *result);

const char *rm_equity_error_message(int error);

#endif /* RIVERMATH_EQUITY_H */
