#ifndef RIVERMATH_BITS_H
#define RIVERMATH_BITS_H

/*
 * Portable wrappers around single-instruction bit operations.
 * The raw builtins are undefined for x == 0; the notes below say which
 * wrappers are safe for 0.
 */

#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define RM_HAVE_BUILTINS 1
#else
#define RM_HAVE_BUILTINS 0
#endif

/* Number of set bits. Safe for 0. */
static inline int rm_popcount64(uint64_t x) {
#if RM_HAVE_BUILTINS
    return __builtin_popcountll(x);
#else
    int n = 0;
    while (x) { x &= x - 1; n++; }
    return n;
#endif
}

/* Index of the lowest set bit. Undefined for 0. */
static inline int rm_ctz64(uint64_t x) {
#if RM_HAVE_BUILTINS
    return __builtin_ctzll(x);
#else
    int n = 0;
    while (!(x & 1)) { x >>= 1; n++; }
    return n;
#endif
}

/* Number of leading zero bits. Undefined for 0. */
static inline int rm_clz64(uint64_t x) {
#if RM_HAVE_BUILTINS
    return __builtin_clzll(x);
#else
    int n = 0;
    while (!(x & (1ULL << 63))) { x <<= 1; n++; }
    return n;
#endif
}

/* Index of the highest set bit (0..63), or -1 for 0. */
static inline int rm_highest_bit(uint64_t x) {
    return x ? 63 - rm_clz64(x) : -1;
}

/* x with its lowest set bit cleared. Safe for 0. */
static inline uint64_t rm_clear_lowest(uint64_t x) {
    return x & (x - 1);
}

#endif /* RIVERMATH_BITS_H */
