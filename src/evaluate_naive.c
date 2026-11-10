/*
 * Naive evaluator: try every 5-card subset of the hand and keep the best.
 *
 * This is the reference the other evaluators are checked against, so it is
 * written to be obviously correct rather than fast: it counts ranks the way a
 * person would and only shares the hand-value encoding and straight helper.
 */

#include "rivermath/evaluate.h"

enum { FIVE_CARDS = 5, MAX_HAND_CARDS = 7 };

/*
 * Order the ranks of a 5-card hand by (how many times they appear, rank),
 * both descending. "K 9 K A 9" becomes groups K, 9, A with sizes 2, 2, 1.
 * Returns the number of groups.
 */
static int group_ranks(const int rank_counts[RM_NUM_RANKS], int group_ranks_out[],
                       int group_sizes_out[]) {
    int num_groups = 0;
    for (int size = 4; size >= 1; size--) {
        for (int rank = RM_RANK_A; rank >= RM_RANK_2; rank--) {
            if (rank_counts[rank] != size) continue;
            group_ranks_out[num_groups] = rank;
            group_sizes_out[num_groups] = size;
            num_groups++;
        }
    }
    return num_groups;
}

static rm_hand_value evaluate_five_cards(const int cards[FIVE_CARDS]) {
    int rank_counts[RM_NUM_RANKS] = {0};
    unsigned rank_mask = 0;
    bool all_same_suit = true;
    for (int i = 0; i < FIVE_CARDS; i++) {
        rank_counts[rm_card_rank(cards[i])]++;
        rank_mask |= 1u << rm_card_rank(cards[i]);
        if (rm_card_suit(cards[i]) != rm_card_suit(cards[0])) all_same_suit = false;
    }

    int ranks[FIVE_CARDS], sizes[FIVE_CARDS];
    int num_groups = group_ranks(rank_counts, ranks, sizes);
    int straight_high = num_groups == FIVE_CARDS ? rm_straight_high_rank(rank_mask) : -1;

    int category;
    if (straight_high >= 0 && all_same_suit) category = RM_STRAIGHT_FLUSH;
    else if (sizes[0] == 4) category = RM_FOUR_OF_A_KIND;
    else if (sizes[0] == 3 && sizes[1] == 2) category = RM_FULL_HOUSE;
    else if (all_same_suit) category = RM_FLUSH;
    else if (straight_high >= 0) category = RM_STRAIGHT;
    else if (sizes[0] == 3) category = RM_THREE_OF_A_KIND;
    else if (sizes[0] == 2 && sizes[1] == 2) category = RM_TWO_PAIR;
    else if (sizes[0] == 2) category = RM_ONE_PAIR;
    else category = RM_HIGH_CARD;

    /* A straight is ranked only by its top card (the wheel's top card is the 5, not the ace). */
    if (category == RM_STRAIGHT || category == RM_STRAIGHT_FLUSH)
        return rm_make_hand_value(category, &straight_high, 1);
    return rm_make_hand_value(category, ranks, num_groups);
}

rm_hand_value rm_evaluate_naive(rm_cards hand) {
    int cards[MAX_HAND_CARDS];
    int num_cards = 0;
    int card;
    while (num_cards < MAX_HAND_CARDS && (card = rm_pop_card(&hand)) >= 0)
        cards[num_cards++] = card;

    rm_hand_value best = 0;
    int subset[FIVE_CARDS];
    for (int a = 0; a < num_cards; a++)
        for (int b = a + 1; b < num_cards; b++)
            for (int c = b + 1; c < num_cards; c++)
                for (int d = c + 1; d < num_cards; d++)
                    for (int e = d + 1; e < num_cards; e++) {
                        subset[0] = cards[a];
                        subset[1] = cards[b];
                        subset[2] = cards[c];
                        subset[3] = cards[d];
                        subset[4] = cards[e];
                        rm_hand_value value = evaluate_five_cards(subset);
                        if (value > best) best = value;
                    }
    return best;
}
