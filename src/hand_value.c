#include "rivermath/evaluate.h"

#include <stdio.h>

static const char *const CATEGORY_NAMES[RM_NUM_CATEGORIES] = {
    "High card", "One pair",  "Two pair",       "Three of a kind", "Straight",
    "Flush",     "Full house", "Four of a kind", "Straight flush",
};

/* Tie-break ranks per category, e.g. two pair = high pair, low pair, kicker. */
static const int NUM_TIEBREAKS[RM_NUM_CATEGORIES] = {5, 4, 3, 3, 1, 5, 2, 2, 1};

rm_hand_value rm_make_hand_value(int category, const int tiebreak_ranks[], int num_tiebreaks) {
    rm_hand_value value = (rm_hand_value)category;
    for (int i = 0; i < RM_MAX_TIEBREAKS; i++) {
        int rank = i < num_tiebreaks ? tiebreak_ranks[i] : 0;
        value = (value << 4) | (rm_hand_value)rank;
    }
    return value;
}

int rm_hand_category(rm_hand_value value) { return (int)(value >> 20); }

const char *rm_category_name(int category) {
    return (category >= 0 && category < RM_NUM_CATEGORIES) ? CATEGORY_NAMES[category] : "?";
}

int rm_num_tiebreaks(int category) {
    return (category >= 0 && category < RM_NUM_CATEGORIES) ? NUM_TIEBREAKS[category] : 0;
}

int rm_straight_high_rank(unsigned rank_mask) {
    /* Shift every rank up one and copy the ace into bit 0, so the ace can also play low. */
    unsigned ace_low = (rank_mask >> RM_RANK_A) & 1u;
    unsigned shifted = (rank_mask << 1) | ace_low;
    /* Bit i survives only if bits i..i+4 are all set: five ranks in a row. */
    unsigned runs = shifted & (shifted >> 1) & (shifted >> 2) & (shifted >> 3) & (shifted >> 4);
    if (runs == 0) return -1;
    /* The run starting at shifted bit i covers ranks i-1 .. i+3, so its top card is rank i+3. */
    return rm_highest_bit(runs) + 3;
}

int rm_describe_hand_value(rm_hand_value value, char *out, size_t out_size) {
    int category = rm_hand_category(value);
    char ranks_text[RM_MAX_TIEBREAKS * 3 + 1] = ""; /* "Q, 4" */
    int length = 0;
    for (int i = 0; i < rm_num_tiebreaks(category); i++) {
        int rank = (int)(value >> (16 - 4 * i)) & 0xF;
        if (i > 0) {
            ranks_text[length++] = ',';
            ranks_text[length++] = ' ';
        }
        ranks_text[length++] = rm_rank_char(rank);
    }
    ranks_text[length] = '\0';
    return snprintf(out, out_size, "%s (%s)", rm_category_name(category), ranks_text);
}
