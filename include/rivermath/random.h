#ifndef RIVERMATH_RANDOM_H
#define RIVERMATH_RANDOM_H

/*
 * Random numbers and random card dealing.
 *
 * Uses xoshiro256** (fast, good quality) seeded through SplitMix64, so the
 * same seed always gives the same cards. That makes Monte Carlo runs and
 * benchmarks reproducible.
 */

#include <stdint.h>

#include "rivermath/card.h"

typedef struct {
    uint64_t state[4];
} rm_random;

void rm_random_seed(rm_random *random, uint64_t seed);

/* Uniform 64-bit number. */
uint64_t rm_random_next(rm_random *random);

/* Uniform number in 0..limit-1 (no modulo bias). limit must be > 0. */
uint32_t rm_random_below(rm_random *random, uint32_t limit);

/*
 * Pick num_cards distinct cards uniformly at random from the available set.
 * Returns them as a set. available must hold at least num_cards cards.
 */
rm_cards rm_deal_random_cards(rm_random *random, rm_cards available, int num_cards);

#endif /* RIVERMATH_RANDOM_H */
