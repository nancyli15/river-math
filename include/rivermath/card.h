#ifndef RIVERMATH_CARD_H
#define RIVERMATH_CARD_H

/*
 * Cards and sets of cards.
 *
 * A single card is a number 0..51:     card = rank * 4 + suit
 * A set of cards is a 64-bit bitmask:  bit  = suit * 16 + rank
 *
 *   bit:  63..48    47..32    31..16    15..0
 *         spades    hearts    diamonds  clubs
 *         (bits 0..12 of each 16-bit lane = ranks 2..A; bits 13..15 unused)
 *
 * Giving each suit its own 16-bit lane means "which ranks do I have in
 * hearts?" is a single shift (see rm_ranks_in_suit).
 *
 * Ranks: 0 = '2' ... 8 = 'T', 9 = 'J', 10 = 'Q', 11 = 'K', 12 = 'A'
 * Suits: 0 = 'c', 1 = 'd', 2 = 'h', 3 = 's'
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "rivermath/bits.h"

/* A set of cards: hole cards, a board, dead cards, a full 7-card hand, ... */
typedef uint64_t rm_cards;

enum {
    RM_NUM_RANKS = 13,
    RM_NUM_SUITS = 4,
    RM_NUM_CARDS = 52,
};

enum {
    RM_RANK_2 = 0, RM_RANK_3, RM_RANK_4, RM_RANK_5, RM_RANK_6, RM_RANK_7,
    RM_RANK_8, RM_RANK_9, RM_RANK_T, RM_RANK_J, RM_RANK_Q, RM_RANK_K, RM_RANK_A,
};

enum { RM_CLUBS = 0, RM_DIAMONDS = 1, RM_HEARTS = 2, RM_SPADES = 3 };

/* Error codes returned by the parse functions (always negative). */
enum {
    RM_ERR_SYNTAX = -1,    /* not a valid card string */
    RM_ERR_DUPLICATE = -2, /* the same card appears twice */
};

#define RM_ALL_RANKS 0x1FFFu /* the 13 rank bits of one suit lane */
#define RM_FULL_DECK                                                   \
    ((rm_cards)RM_ALL_RANKS | ((rm_cards)RM_ALL_RANKS << 16) |         \
     ((rm_cards)RM_ALL_RANKS << 32) | ((rm_cards)RM_ALL_RANKS << 48))

/* ---- single cards ---------------------------------------------------- */

static inline int rm_make_card(int rank, int suit) { return rank * 4 + suit; }
static inline int rm_card_rank(int card) { return card / 4; }
static inline int rm_card_suit(int card) { return card % 4; }
static inline bool rm_is_valid_card(int card) { return card >= 0 && card < RM_NUM_CARDS; }

/* Bit position of a card inside an rm_cards set. */
static inline int rm_card_to_bit(int card) {
    return rm_card_suit(card) * 16 + rm_card_rank(card);
}

static inline int rm_bit_to_card(int bit) {
    int rank = bit % 16;
    int suit = bit / 16;
    return rm_make_card(rank, suit);
}

/* A set containing just this card. */
static inline rm_cards rm_card_set(int card) {
    return 1ULL << rm_card_to_bit(card);
}

/* ---- sets of cards ----------------------------------------------------- */

static inline int rm_count_cards(rm_cards cards) { return rm_popcount64(cards); }

/* True if only real card bits are set (none of the unused bits 13..15 of a lane). */
static inline bool rm_is_valid_card_set(rm_cards cards) {
    return (cards & ~RM_FULL_DECK) == 0;
}

/* 13-bit mask of the ranks present in one suit, e.g. bit 12 set = has the ace. */
static inline unsigned rm_ranks_in_suit(rm_cards cards, int suit) {
    return (unsigned)(cards >> (16 * suit)) & RM_ALL_RANKS;
}

/*
 * Remove the lowest card from *cards and return it, or -1 if the set is empty.
 * Loop over a set with:  while ((card = rm_pop_card(&remaining)) >= 0) { ... }
 */
static inline int rm_pop_card(rm_cards *cards) {
    if (*cards == 0) return -1;
    int bit = rm_ctz64(*cards);
    *cards = rm_clear_lowest(*cards);
    return rm_bit_to_card(bit);
}

/* ---- text ------------------------------------------------------------- */

/* Rank/suit characters: "23456789TJQKA" and "cdhs". */
char rm_rank_char(int rank);
char rm_suit_char(int suit);

/*
 * Parse one card from the first two characters of text, e.g. "Ah", "td".
 * Rank and suit are case-insensitive. Returns 0..51 or RM_ERR_SYNTAX.
 */
int rm_parse_card(const char *text);

/*
 * Parse a list of cards such as "AhKd", "Ah Kd 2c", "ah,kd".
 * Spaces and commas between cards are ignored, and "" is a valid empty set.
 * On success stores the set in *out and returns the number of cards.
 * On failure returns RM_ERR_SYNTAX or RM_ERR_DUPLICATE and leaves *out unchanged.
 */
int rm_parse_cards(const char *text, rm_cards *out);

/* Write a card as two characters plus NUL, e.g. "Ah". Returns out. */
char *rm_format_card(int card, char out[3]);

/*
 * Write a set of cards, highest rank first (suits in c d h s order within a
 * rank), with no separators, e.g. "AhKdTc". Works like snprintf: writes at most
 * out_size bytes including the NUL and returns the full length needed
 * (excluding the NUL).
 */
size_t rm_format_cards(rm_cards cards, char *out, size_t out_size);

#endif /* RIVERMATH_CARD_H */
