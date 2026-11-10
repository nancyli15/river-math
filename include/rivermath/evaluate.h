#ifndef RIVERMATH_EVALUATE_H
#define RIVERMATH_EVALUATE_H

/*
 * Hand evaluation: turn 5 to 7 cards into one number, the hand value.
 * A bigger hand value is a better poker hand, so comparing two hands is just `>`.
 *
 * Hand value layout (24 bits):
 *
 *   category << 20 | r1 << 16 | r2 << 12 | r3 << 8 | r4 << 4 | r5
 *
 * category is RM_HIGH_CARD..RM_STRAIGHT_FLUSH, and r1..r5 are the tie-break
 * ranks in order of importance (CONCEPTS.md Part 2). For example, a full house
 * of queens over fours has category RM_FULL_HOUSE, r1 = Q, r2 = 4. Unused
 * slots are 0.
 */

#include <stdint.h>

#include "rivermath/card.h"

typedef uint32_t rm_hand_value;

enum {
    RM_HIGH_CARD = 0,
    RM_ONE_PAIR,
    RM_TWO_PAIR,
    RM_THREE_OF_A_KIND,
    RM_STRAIGHT,
    RM_FLUSH,
    RM_FULL_HOUSE,
    RM_FOUR_OF_A_KIND,
    RM_STRAIGHT_FLUSH,
    RM_NUM_CATEGORIES,
};

enum { RM_MAX_TIEBREAKS = 5 };

/* Every hand value is below this, so it can size lookup arrays. */
#define RM_HAND_VALUE_LIMIT ((rm_hand_value)RM_NUM_CATEGORIES << 20)

/* ---- shared helpers (hand_value.c) ------------------------------------ */

/* Build a hand value from a category and its tie-break ranks (most important first). */
rm_hand_value rm_make_hand_value(int category, const int tiebreak_ranks[], int num_tiebreaks);

int rm_hand_category(rm_hand_value value);
const char *rm_category_name(int category);

/* How many tie-break ranks a category uses, e.g. 2 for a full house. */
int rm_num_tiebreaks(int category);

/*
 * Rank of the top card of the best straight in a 13-bit rank mask, or -1 if
 * there is no straight. A-2-3-4-5 (the wheel) counts, with top card 5.
 */
int rm_straight_high_rank(unsigned rank_mask);

/* Write e.g. "Full house (Q, 4)". Works like snprintf. */
int rm_describe_hand_value(rm_hand_value value, char *out, size_t out_size);

/* ---- evaluators ---------------------------------------------------------- */

/* Reference evaluator: best of every 5-card subset. Works for 5 to 7 cards. */
rm_hand_value rm_evaluate_naive(rm_cards hand);

/* Fast evaluator: all cards at once with bit tricks. Works for 5 to 7 cards. */
rm_hand_value rm_evaluate_bitwise(rm_cards hand);

#endif /* RIVERMATH_EVALUATE_H */
