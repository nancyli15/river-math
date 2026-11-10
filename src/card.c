#include "rivermath/card.h"

#include <ctype.h>

static const char RANK_CHARS[] = "23456789TJQKA";
static const char SUIT_CHARS[] = "cdhs";

char rm_rank_char(int rank) {
    return (rank >= 0 && rank < RM_NUM_RANKS) ? RANK_CHARS[rank] : '?';
}

char rm_suit_char(int suit) {
    return (suit >= 0 && suit < RM_NUM_SUITS) ? SUIT_CHARS[suit] : '?';
}

/* Position of c in chars (case-insensitive), or -1. */
static int index_of_char(const char *chars, int num_chars, char c) {
    for (int i = 0; i < num_chars; i++)
        if (tolower((unsigned char)chars[i]) == tolower((unsigned char)c)) return i;
    return -1;
}

int rm_parse_card(const char *text) {
    if (!text || text[0] == '\0') return RM_ERR_SYNTAX;
    int rank = index_of_char(RANK_CHARS, RM_NUM_RANKS, text[0]);
    int suit = index_of_char(SUIT_CHARS, RM_NUM_SUITS, text[1]); /* '\0' is never found */
    if (rank < 0 || suit < 0) return RM_ERR_SYNTAX;
    return rm_make_card(rank, suit);
}

static bool is_separator(char c) {
    return c == ',' || isspace((unsigned char)c);
}

int rm_parse_cards(const char *text, rm_cards *out) {
    if (!text || !out) return RM_ERR_SYNTAX;
    rm_cards cards = 0;
    int num_cards = 0;
    while (*text) {
        if (is_separator(*text)) {
            text++;
            continue;
        }
        int card = rm_parse_card(text);
        if (card < 0) return RM_ERR_SYNTAX;
        if (cards & rm_card_set(card)) return RM_ERR_DUPLICATE;
        cards |= rm_card_set(card);
        num_cards++;
        text += 2;
    }
    *out = cards;
    return num_cards;
}

char *rm_format_card(int card, char out[3]) {
    bool valid = rm_is_valid_card(card);
    out[0] = valid ? rm_rank_char(rm_card_rank(card)) : '?';
    out[1] = valid ? rm_suit_char(rm_card_suit(card)) : '?';
    out[2] = '\0';
    return out;
}

/* Append c if there is room, always leaving space for the final NUL. */
static void append_char(char *out, size_t out_size, size_t *length, char c) {
    if (*length + 1 < out_size) out[*length] = c;
    (*length)++;
}

size_t rm_format_cards(rm_cards cards, char *out, size_t out_size) {
    size_t length = 0;
    for (int rank = RM_NUM_RANKS - 1; rank >= 0; rank--) {
        for (int suit = 0; suit < RM_NUM_SUITS; suit++) {
            if (!(cards & rm_card_set(rm_make_card(rank, suit)))) continue;
            append_char(out, out_size, &length, rm_rank_char(rank));
            append_char(out, out_size, &length, rm_suit_char(suit));
        }
    }
    if (out_size > 0) out[length < out_size ? length : out_size - 1] = '\0';
    return length;
}
