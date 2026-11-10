/*
 * Bitwise evaluator: score all 5 to 7 cards at once using the four 13-bit
 * suit masks, with no loops over subsets and no tables.
 * The bit tricks are explained in CONCEPTS.md §7.3.
 */

#include "rivermath/evaluate.h"

/* Rank masks grouped by how many times each rank appears in the hand. */
typedef struct {
    unsigned any;   /* rank appears at least once */
    unsigned twice; /* exactly 2 */
    unsigned three; /* exactly 3 */
    unsigned four;  /* exactly 4 */
} rank_groups;

static rank_groups group_ranks_by_count(rm_cards hand) {
    unsigned clubs = rm_ranks_in_suit(hand, RM_CLUBS);
    unsigned diamonds = rm_ranks_in_suit(hand, RM_DIAMONDS);
    unsigned hearts = rm_ranks_in_suit(hand, RM_HEARTS);
    unsigned spades = rm_ranks_in_suit(hand, RM_SPADES);

    unsigned odd_count = clubs ^ diamonds ^ hearts ^ spades; /* appears 1 or 3 times */
    unsigned at_least_two = (clubs & diamonds) | (hearts & spades) |
                            ((clubs | diamonds) & (hearts | spades));

    rank_groups groups;
    groups.any = clubs | diamonds | hearts | spades;
    groups.four = clubs & diamonds & hearts & spades;
    groups.three = odd_count & at_least_two;
    groups.twice = at_least_two & ~odd_count & ~groups.four;
    return groups;
}

/* Remove the highest rank from *rank_mask and return it. *rank_mask must not be 0. */
static int pop_highest_rank(unsigned *rank_mask) {
    int rank = rm_highest_bit(*rank_mask);
    *rank_mask &= ~(1u << rank);
    return rank;
}

/* Hand value for a category given its leading ranks, filling the rest with the best kickers. */
static rm_hand_value make_value_with_kickers(int category, int ranks[], int num_leading,
                                             unsigned kicker_mask) {
    int num_tiebreaks = rm_num_tiebreaks(category);
    for (int i = num_leading; i < num_tiebreaks; i++)
        ranks[i] = pop_highest_rank(&kicker_mask);
    return rm_make_hand_value(category, ranks, num_tiebreaks);
}

/* The suit with 5+ cards, or -1. With at most 7 cards only one suit can have 5. */
static int find_flush_suit(rm_cards hand) {
    for (int suit = 0; suit < RM_NUM_SUITS; suit++)
        if (rm_popcount64(rm_ranks_in_suit(hand, suit)) >= 5) return suit;
    return -1;
}

rm_hand_value rm_evaluate_bitwise(rm_cards hand) {
    int ranks[RM_MAX_TIEBREAKS];

    /*
     * With 7 or fewer cards, a flush rules out quads and full houses (only 2
     * cards would be left over), so the flush can be decided first.
     */
    int flush_suit = find_flush_suit(hand);
    if (flush_suit >= 0) {
        unsigned flush_ranks = rm_ranks_in_suit(hand, flush_suit);
        ranks[0] = rm_straight_high_rank(flush_ranks);
        if (ranks[0] >= 0) return rm_make_hand_value(RM_STRAIGHT_FLUSH, ranks, 1);
        return make_value_with_kickers(RM_FLUSH, ranks, 0, flush_ranks);
    }

    rank_groups groups = group_ranks_by_count(hand);

    if (groups.four) {
        ranks[0] = rm_highest_bit(groups.four);
        return make_value_with_kickers(RM_FOUR_OF_A_KIND, ranks, 1, groups.any & ~groups.four);
    }

    if (groups.three) {
        unsigned threes = groups.three;
        ranks[0] = pop_highest_rank(&threes);
        /* A second set of three can supply the pair. */
        unsigned pair_candidates = threes | groups.twice;
        if (pair_candidates) {
            ranks[1] = rm_highest_bit(pair_candidates);
            return rm_make_hand_value(RM_FULL_HOUSE, ranks, 2);
        }
    }

    ranks[0] = rm_straight_high_rank(groups.any);
    if (ranks[0] >= 0) return rm_make_hand_value(RM_STRAIGHT, ranks, 1);

    if (groups.three) {
        ranks[0] = rm_highest_bit(groups.three);
        return make_value_with_kickers(RM_THREE_OF_A_KIND, ranks, 1,
                                       groups.any & ~groups.three);
    }

    if (rm_popcount64(groups.twice) >= 2) {
        unsigned pairs = groups.twice;
        ranks[0] = pop_highest_rank(&pairs);
        ranks[1] = pop_highest_rank(&pairs);
        /* A third pair's rank can still be the kicker. */
        unsigned used = (1u << ranks[0]) | (1u << ranks[1]);
        return make_value_with_kickers(RM_TWO_PAIR, ranks, 2, groups.any & ~used);
    }

    if (groups.twice) {
        ranks[0] = rm_highest_bit(groups.twice);
        return make_value_with_kickers(RM_ONE_PAIR, ranks, 1, groups.any & ~groups.twice);
    }

    return make_value_with_kickers(RM_HIGH_CARD, ranks, 0, groups.any);
}
