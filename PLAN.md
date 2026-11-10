# RiverMath: Project Plan

A fast Texas Hold'em hand evaluator and multi-player equity calculator in C.

Coding rules live in [CLAUDE.md](CLAUDE.md). Background on poker, math and algorithms is in [CONCEPTS.md](CONCEPTS.md).

---

## 1. v1: minimum to make the resume true

The goal of v1 is the **smallest app that makes every resume bullet true**. Everything else is an expansion (§7).

| Resume claim | v1 builds exactly this | Proven by |
|---|---|---|
| C evaluator with 64-bit bitmasks, **10M+ hands/sec** | Bitwise evaluator + `rivermath bench` | Measured number in README |
| **3 evaluation algorithms** (combinatorial, bitwise, perfect-hash) | Naive, bitwise and lookup evaluators behind one API | Identical results on all 133,784,560 7-card hands |
| **9-player equity**, exact enumeration + **1M+ Monte Carlo** | One `rm_equity` function: exact when all hands are known, Monte Carlo (default 1M samples) when any player is "random" | Known matchups; MC within tolerance of exact |
| **Automated correctness testing** | `make test` + CI | GitHub Actions badge |

**v1 file list (complete):**
```
include/rivermath/  bits.h  card.h  evaluate.h  equity.h
src/                card.c  evaluate_naive.c  evaluate_bitwise.c  evaluate_lookup.c  hand_value.c  equity.c
apps/cli/main.c     commands: cards, eval, equity, bench
tests/              test_card.c  test_evaluate.c  test_equity.c
```

**Not in v1** (deliberately): ranges, threads, confidence intervals, early stopping, JSON output, REPL, web UI,
build-time table generator, separate bench binary.

**Rule:** resume and README numbers come only from `rivermath bench` output.

---

## 2. Tech stack

- **Language:** C17. No dependencies.
- **Build:** plain GNU Make (`make`, `make test`, `make run ARGS="..."`). Release builds go to `build/release/`, and debug+sanitizer builds to `build/debug/`.
- **Compilers:** clang (macOS) and gcc (CI). Use `-O3` for release and `-O1 -g -fsanitize=address,undefined` for tests.
- **Bit ops:** `__builtin_popcountll`, `__builtin_ctzll`, `__builtin_clzll`, wrapped in `bits.h`.
- **RNG:** xoshiro256** seeded with SplitMix64 (about 20 lines, a private part of `equity.c`). Don't use `rand()`.
- **Tests:** our own small harness (`tests/test.h`).
- **Timing:** `clock_gettime(CLOCK_MONOTONIC)`.
- **CI:** GitHub Actions running gcc + clang on Ubuntu and clang on macOS.

---

## 3. Core design decisions

### Card representation (done in M0)
- A single card is `0..51` = `rank * 4 + suit`.
- A set of cards is `rm_cards` (`uint64_t`) with bit `suit * 16 + rank`. Each suit gets a 16-bit lane, so
  `rm_ranks_in_suit(cards, suit)` is a single shift and mask.
- Union is `a | b`, and overlap is `a & b`.

### Hand value (shared by all evaluators): `hand_value.c`
- `rm_hand_value` (`uint32_t`): **a bigger number means a better hand**. Any two values compare with `>`.
- Layout: `category << 20 | r1 << 16 | r2 << 12 | r3 << 8 | r4 << 4 | r5`.
  `category` is 0 = high card … 8 = straight flush, and `r1..r5` are the tie-break ranks in order of importance
  (e.g. full house: r1 = trips rank, r2 = pair rank).
- **One** helper builds this value, and **one** helper finds the high card of a straight in a 13-bit rank mask.
  Every evaluator uses these two helpers.
- Sanity check: there are exactly 7,462 distinct values over all 5-card hands.

### Three evaluators
1. **Naive (`evaluate_naive.c`)**: tries all C(7,5) = 21 five-card subsets, scores each by counting ranks and suits, and keeps the best.
   It's the independent reference used to check the other two.
2. **Bitwise (`evaluate_bitwise.c`)**: evaluates all 7 cards at once from the bitmask (bit tricks in CONCEPTS.md §7.3). No tables.
3. **Lookup / perfect hash (`evaluate_lookup.c`)**, for exactly 7 cards:
   - **Flush table:** 8,192 entries indexed by the flush suit's 13-bit rank mask. (A flush rules out quads and full houses in 7 cards.)
   - **Non-flush:** each rank has a weight (`0, 1, 5, 22, 98, 453, 2031, 8698, 22854, …`) chosen so that every
     7-card rank combination sums to a **unique key**. There are 49,205 combinations, but the largest key is about 7.8M,
     so a single-level table would be about 31 MB. A small **displacement (offset) table** packs the keys into roughly 50K slots with no collisions.
   - The tables are **filled at startup by calling the bitwise evaluator**. That means there's no ranking logic to duplicate and no generator tool.
     Building them checks that every slot is unique (no collisions).

### Equity (`equity.c`)
- Input: 2–9 players (each one either known hole cards or "random"), a board of 0/3/4/5 cards, and dead cards.
- **All hands known → exact:** enumerate every way to finish the board. For example, 9 known hands preflop gives C(34,5) = 278,256 boards.
- **Any random player → Monte Carlo:** each sample deals the random hole cards and the missing board cards (partial Fisher–Yates). Default is 1,000,000 samples, with a seed for reproducibility.
- Split pots: each of k tied winners gets 1/k. Output is the equity per player, plus win% and tie%.
- One private helper, `score_one_deal`, evaluates every player for one finished board and adds up the shares. Both exact and MC call it.

### CLI
```
rivermath cards Ah Kd 2c                         (done)
rivermath eval AhKhQhJhTh2c3d                    -> Straight flush, A high
rivermath equity AhAs KdKc --board 2h7c9s        -> exact
rivermath equity AhAs random random --samples 1000000 --seed 42   -> Monte Carlo
rivermath bench                                  -> hands/sec for each evaluator
```

### Public API (v1)
```c
// card.h (done)
typedef uint64_t rm_cards;
int    rm_parse_cards(const char *text, rm_cards *out);
size_t rm_format_cards(rm_cards cards, char *out, size_t out_size);

// evaluate.h
typedef uint32_t rm_hand_value;                       // bigger = better
rm_hand_value rm_evaluate_naive(rm_cards hand);       // 5 to 7 cards
rm_hand_value rm_evaluate_bitwise(rm_cards hand);     // 5 to 7 cards
rm_hand_value rm_evaluate_lookup(rm_cards hand);      // exactly 7 cards; tables built on first call
int           rm_hand_category(rm_hand_value value);  // 0..8
const char   *rm_category_name(int category);         // "Full house"

// equity.h
enum { RM_MAX_PLAYERS = 9 };
typedef struct {
    int      num_players;                  // 2..9
    rm_cards hole_cards[RM_MAX_PLAYERS];   // 0 = random
    rm_cards board;                        // 0, 3, 4 or 5 cards
    rm_cards dead_cards;
    uint64_t num_samples;                  // Monte Carlo only; 0 = default (1,000,000)
    uint64_t seed;
} rm_equity_input;

typedef struct {
    double   equity[RM_MAX_PLAYERS];       // 0..1, ties shared
    double   win[RM_MAX_PLAYERS];
    double   tie[RM_MAX_PLAYERS];
    uint64_t num_deals;                    // boards enumerated or samples taken
    bool     was_exact;
} rm_equity_result;

int rm_equity(const rm_equity_input *input, rm_equity_result *result);   // 0 or negative error
```

---

## 4. Correctness strategy

**All 5-card hands (2,598,960):**
| Category | Count |
|---|---|
| Straight flush (incl. royal) | 40 |
| Four of a kind | 624 |
| Full house | 3,744 |
| Flush | 5,108 |
| Straight | 10,200 |
| Three of a kind | 54,912 |
| Two pair | 123,552 |
| One pair | 1,098,240 |
| High card | 1,302,540 |
| Distinct values | 7,462 |

**All 7-card hands (133,784,560):**
| Category | Count |
|---|---|
| Straight flush | 41,584 |
| Four of a kind | 224,848 |
| Full house | 3,473,184 |
| Flush | 4,047,644 |
| Straight | 6,180,020 |
| Three of a kind | 6,461,620 |
| Two pair | 31,433,400 |
| One pair | 58,627,800 |
| High card | 23,294,460 |
| Distinct values | 4,824 |

Tests:
1. Edge cases from CONCEPTS.md Part 2 (wheel, no wrap-around, three pairs, two trips, 6-card flush, etc.).
2. 5-card category counts for naive and bitwise (fast).
3. 7-card: the bitwise category counts, and **naive == bitwise == lookup on all 133M hands**. This is slow under sanitizers, so it runs as `make test-full` (release build) and in CI.
4. Equity: AA vs KK preflop is about 82/18, plus a few hand-checkable spots. One example: a river board where both players play the board must give exactly 50/50.
5. Monte Carlo vs exact on the same spot, with a fixed seed: within 0.5%.

---

## 5. Repository structure (v1)

```
river-math/
├── CLAUDE.md  PLAN.md  CONCEPTS.md  README.md  Makefile
├── include/rivermath/   bits.h  card.h  evaluate.h  equity.h
├── src/                 card.c  hand_value.c  evaluate_naive.c  evaluate_bitwise.c  evaluate_lookup.c  equity.c
├── apps/cli/main.c
├── tests/               test.h  test_main.c  test_bits.c  test_card.c  test_evaluate.c  test_equity.c
└── .github/workflows/ci.yml
```

---

## 6. Milestones (v1)

### M0: Scaffolding ✅
- [x] Makefile, warnings-as-errors
- [x] Test harness, CI workflow, `.gitignore`, README
- [x] `card.c`: parse/format, bitmask helpers, plus tests

### M1: Two evaluators
- [ ] `hand_value.c`: value encoding, category names, straight-high helper
- [ ] Naive evaluator
- [ ] Bitwise evaluator
- [ ] Edge-case tests, 5-card counts, 7-card counts + naive == bitwise (`make test-full`)
- [ ] `rivermath eval`

### M2: Lookup evaluator + benchmark
- [ ] Rank weights + displacement table, built at startup from the bitwise evaluator
- [ ] Cross-check lookup == bitwise on all 7-card hands
- [ ] `rivermath bench`: hands/sec for each evaluator on the same shuffled set of hands
- [ ] Benchmark table in README (machine + compiler noted)

### M3: Equity
- [ ] `rm_equity`: exact enumeration (all hands known) and Monte Carlo (any random player)
- [ ] RNG + card dealing helper
- [ ] Equity tests (reference matchups, board-plays 50/50, MC vs exact)
- [ ] `rivermath equity`
- [ ] Fill in the resume numbers → **v1 done**

---

## 7. Expansions (after v1, only when wanted)

| Expansion | What it adds |
|---|---|
| Ranges | `QQ+, AKs, T9s-76s` syntax, range vs range |
| Threads | pthreads for MC and enumeration, plus a scaling chart |
| Statistics | 95% confidence intervals, stopping early once the target error is reached |
| Faster enumeration | Evaluate the board once and add each player's hole cards to it |
| Web demo | Emscripten → WASM page with a card picker and equity by street (preflop → river), hosted on GitHub Pages |
| Extras | JSON output, REPL, outs calculator, Python bindings, Omaha |

---

## 8. Resume wording (fill in after M3)

> **RiverMath: Poker Hand Evaluator & Equity Calculator** (C)
> - Built a Texas Hold'em evaluator in C using 64-bit bitmasks and optimized hand-ranking algorithms, reaching **___M hands/sec** single-threaded.
> - Implemented 3 evaluation algorithms (combinatorial, bitwise, perfect-hash lookup), verified identical across all 133.8M 7-card hands.
> - Developed a 9-player equity calculator using exact board enumeration and 1M+ Monte Carlo simulations, with automated correctness testing in CI.
