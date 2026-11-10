/*
 * Lookup evaluator: answer each 7-card hand with table reads.
 *
 * Two tables, both filled once at startup by calling the bitwise evaluator, so
 * no hand-ranking rules are repeated here:
 *
 * 1. Flush table (8,192 entries), indexed directly by the 13-bit rank mask of
 *    the flush suit. In 7 cards a flush rules out quads and full houses, so
 *    this mask alone decides the hand.
 *
 * 2. Rank table. Without a flush, suits don't matter, only how many of each
 *    rank you hold. Each rank gets a weight, chosen so every 7-card rank
 *    combination (49,205 of them) adds up to a different key. The keys go up
 *    to about 7.8 million, so a table indexed by key would be about 31 MB.
 *    A displacement table packs them tightly instead:
 *
 *        slot = (key % ROW_SIZE) + row_offset[key / ROW_SIZE]
 *
 *    Each row of keys gets an offset chosen so its keys land in free slots.
 *    No two hand values ever share a slot, which makes it a perfect hash.
 */

#include <stdio.h>
#include <stdlib.h>

#include "rivermath/evaluate.h"

enum {
    HAND_SIZE = 7,
    NUM_RANK_COMBOS = 49205, /* ways to hold 7 cards' ranks, each rank at most 4 times */
};

/* Rank weights: any 7 ranks (each used at most 4 times) have a unique sum. */
static const uint32_t RANK_WEIGHTS[RM_NUM_RANKS] = {
    0, 1, 5, 22, 98, 453, 2031, 8698, 22854, 83661, 262349, 636345, 1479181,
};

#define MAX_KEY (4 * 1479181 + 3 * 636345) /* four aces + three kings */
#define ROW_BITS 9
#define ROW_SIZE (1 << ROW_BITS)
#define NUM_ROWS (MAX_KEY / ROW_SIZE + 1)
#define EMPTY_SLOT 0 /* no 7-card hand has value 0 */

static rm_hand_value flush_table[1 << RM_NUM_RANKS];
static uint32_t mask_key[1 << RM_NUM_RANKS]; /* sum of rank weights for each 13-bit mask */
static uint32_t row_offset[NUM_ROWS];
static rm_hand_value *rank_table;
static uint32_t rank_table_size;
static bool tables_built;

/* ---- building the tables ------------------------------------------------- */

/* Table building can only fail if the constants above are wrong: a programming error. */
static void fail(const char *reason) {
    fprintf(stderr, "rivermath: cannot build lookup tables: %s\n", reason);
    abort();
}

static void build_flush_table(void) {
    for (unsigned ranks = 0; ranks < (1u << RM_NUM_RANKS); ranks++) {
        if (rm_popcount64(ranks) < 5) continue;
        rm_cards hearts = (rm_cards)ranks << (16 * RM_HEARTS);
        flush_table[ranks] = rm_evaluate_bitwise(hearts);
    }
}

static void build_mask_keys(void) {
    for (unsigned ranks = 0; ranks < (1u << RM_NUM_RANKS); ranks++) {
        uint32_t key = 0;
        for (int rank = 0; rank < RM_NUM_RANKS; rank++)
            if (ranks & (1u << rank)) key += RANK_WEIGHTS[rank];
        mask_key[ranks] = key;
    }
}

/* Every 7-card rank combination as (key, hand value), with no flush possible. */
typedef struct {
    uint32_t key;
    rm_hand_value value;
} key_value;

static int num_combos;
static key_value combos[NUM_RANK_COMBOS];

/* Fill in rank counts from `rank` upward, then record the finished combination. */
static void collect_rank_combos(int rank, int cards_left, int rank_counts[]) {
    if (rank == RM_NUM_RANKS) {
        if (cards_left > 0) return;
        /* Deal suits round-robin so no suit gets more than 2 cards: never a flush. */
        rm_cards hand = 0;
        uint32_t key = 0;
        int card_number = 0;
        for (int r = 0; r < RM_NUM_RANKS; r++)
            for (int copy = 0; copy < rank_counts[r]; copy++) {
                hand |= rm_card_set(rm_make_card(r, card_number++ % RM_NUM_SUITS));
                key += RANK_WEIGHTS[r];
            }
        if (num_combos == NUM_RANK_COMBOS) fail("more rank combinations than expected");
        combos[num_combos].key = key;
        combos[num_combos].value = rm_evaluate_bitwise(hand);
        num_combos++;
        return;
    }
    for (int count = 0; count <= 4 && count <= cards_left; count++) {
        rank_counts[rank] = count;
        collect_rank_combos(rank + 1, cards_left - count, rank_counts);
    }
}

/* Sort rows so the most crowded are placed first; they are the hardest to fit. */
static int row_sizes[NUM_ROWS];
static int compare_rows_by_size(const void *a, const void *b) {
    return row_sizes[*(const int *)b] - row_sizes[*(const int *)a];
}

static int compare_combos_by_key(const void *a, const void *b) {
    uint32_t key_a = ((const key_value *)a)->key, key_b = ((const key_value *)b)->key;
    return (key_a > key_b) - (key_a < key_b);
}

/* Can a row whose keys start at combos[first] be placed at this offset? */
static bool row_fits(const rm_hand_value *slots, uint32_t capacity, int first, int count,
                     uint32_t offset) {
    for (int i = first; i < first + count; i++) {
        uint32_t slot = (combos[i].key % ROW_SIZE) + offset;
        if (slot >= capacity) return false;
        rm_hand_value existing = slots[slot];
        if (existing != EMPTY_SLOT && existing != combos[i].value) return false;
    }
    return true;
}

static void build_rank_table(void) {
    int rank_counts[RM_NUM_RANKS] = {0};
    collect_rank_combos(0, HAND_SIZE, rank_counts);
    if (num_combos != NUM_RANK_COMBOS) fail("fewer rank combinations than expected");
    qsort(combos, (size_t)num_combos, sizeof combos[0], compare_combos_by_key);
    for (int i = 1; i < num_combos; i++)
        if (combos[i].key == combos[i - 1].key) fail("two rank combinations share a key");

    /* combos is sorted by key, so each row's keys are one contiguous run. */
    static int row_start[NUM_ROWS], rows_by_size[NUM_ROWS];
    for (int i = num_combos - 1; i >= 0; i--) {
        int row = (int)(combos[i].key / ROW_SIZE);
        row_start[row] = i;
        row_sizes[row]++;
    }
    for (int row = 0; row < NUM_ROWS; row++) rows_by_size[row] = row;
    qsort(rows_by_size, NUM_ROWS, sizeof rows_by_size[0], compare_rows_by_size);

    /* Generous working space; it is trimmed to the size actually used at the end. */
    uint32_t capacity = 4 * (uint32_t)num_combos + ROW_SIZE;
    rm_hand_value *slots = calloc(capacity, sizeof *slots);
    if (!slots) fail("out of memory");
    for (int i = 0; i < NUM_ROWS && row_sizes[rows_by_size[i]] > 0; i++) {
        int row = rows_by_size[i];
        uint32_t offset = 0;
        while (!row_fits(slots, capacity, row_start[row], row_sizes[row], offset))
            if (++offset >= capacity) fail("rows do not fit");
        for (int c = row_start[row]; c < row_start[row] + row_sizes[row]; c++) {
            uint32_t slot = (combos[c].key % ROW_SIZE) + offset;
            slots[slot] = combos[c].value;
            if (slot + 1 > rank_table_size) rank_table_size = slot + 1;
        }
        row_offset[row] = offset;
    }
    rank_table = realloc(slots, rank_table_size * sizeof *slots);
    if (!rank_table) fail("out of memory");
}

static void build_tables(void) {
    build_flush_table();
    build_mask_keys();
    build_rank_table();
    tables_built = true;
}

/* ---- evaluating ------------------------------------------------------------- */

rm_hand_value rm_evaluate_lookup(rm_cards hand) {
    if (!tables_built) build_tables(); /* not thread-safe; v1 is single-threaded */
    if (rm_count_cards(hand) != HAND_SIZE) return rm_evaluate_bitwise(hand);

    uint32_t key = 0;
    for (int suit = 0; suit < RM_NUM_SUITS; suit++) {
        unsigned ranks = rm_ranks_in_suit(hand, suit);
        if (rm_popcount64(ranks) >= 5) return flush_table[ranks];
        key += mask_key[ranks];
    }
    return rank_table[(key % ROW_SIZE) + row_offset[key / ROW_SIZE]];
}

size_t rm_lookup_table_bytes(void) {
    if (!tables_built) build_tables();
    return sizeof flush_table + sizeof mask_key + sizeof row_offset +
           rank_table_size * sizeof(rm_hand_value);
}
