#include "rivermath/random.h"

static uint64_t rotate_left(uint64_t x, int bits) {
    return (x << bits) | (x >> (64 - bits));
}

/* SplitMix64 spreads one seed into well-mixed starting state. */
static uint64_t splitmix64(uint64_t *seed) {
    uint64_t z = (*seed += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

void rm_random_seed(rm_random *random, uint64_t seed) {
    for (int i = 0; i < 4; i++) random->state[i] = splitmix64(&seed);
}

/* xoshiro256** by Blackman and Vigna. */
uint64_t rm_random_next(rm_random *random) {
    uint64_t *s = random->state;
    uint64_t result = rotate_left(s[1] * 5, 7) * 9;
    uint64_t shifted = s[1] << 17;
    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= shifted;
    s[3] = rotate_left(s[3], 45);
    return result;
}

/*
 * Lemire's method: multiply a random 32-bit number by limit and keep the top
 * 32 bits. Rejecting a few low results removes the slight bias that
 * `random % limit` would have.
 */
uint32_t rm_random_below(rm_random *random, uint32_t limit) {
    uint32_t biased_zone = (uint32_t)(-limit) % limit;
    for (;;) {
        uint64_t product = (rm_random_next(random) >> 32) * (uint64_t)limit;
        if ((uint32_t)product >= biased_zone) return (uint32_t)(product >> 32);
    }
}

rm_cards rm_deal_random_cards(rm_random *random, rm_cards available, int num_cards) {
    int deck[RM_NUM_CARDS];
    int deck_size = 0;
    int card;
    while ((card = rm_pop_card(&available)) >= 0) deck[deck_size++] = card;

    /* Partial Fisher-Yates shuffle: only the first num_cards positions are needed. */
    rm_cards dealt = 0;
    for (int i = 0; i < num_cards && i < deck_size; i++) {
        int pick = i + (int)rm_random_below(random, (uint32_t)(deck_size - i));
        int picked_card = deck[pick];
        deck[pick] = deck[i];
        deck[i] = picked_card;
        dealt |= rm_card_set(picked_card);
    }
    return dealt;
}
