# RiverMath: Concepts Guide

This guide covers everything you need to know to build this project. It's ordered the way you'll need it,
starting with how poker works and ending with the performance details.

| Part | Topic | Needed for |
|---|---|---|
| 1 | How the game works | M0 to M1 |
| 2 | Hand rankings and tie-breaking | M1 |
| 3 | Poker vocabulary for this project | M1, M3, M4 |
| 4 | Strategy: why anyone wants equity | Context, test cases, the M6 UI |
| 5 | Combinatorics and probability | M1 to M4 |
| 6 | C and bit-manipulation basics | M0 to M2 |
| 7 | Algorithms | M1 to M5 |
| 8 | Statistics for Monte Carlo | M3 |
| 9 | Performance and benchmarking | M2, M5 |
| 10 | Glossary | Quick reference |

> You don't need to be good at poker to build this. You need to know the rules exactly (Parts 1–2)
> and the math (Part 5). Strategy (Part 4) is mostly there so the tool makes sense to you, and it
> also gives you known numbers to test against.

---

## Part 1: How the game works (Texas Hold'em)

### The deck
- There are 52 cards: **13 ranks** × **4 suits**.
- Ranks from low to high: `2 3 4 5 6 7 8 9 T J Q K A`. "T" stands for 10, so every rank is one character.
- Suits: clubs `c` ♣, diamonds `d` ♦, hearts `h` ♥, spades `s` ♠.
- **Suits have no rank.** No suit beats another. Suits only matter for flushes.
- A card is written as rank + suit, e.g. `Ah` (ace of hearts), `Tc` (ten of clubs).

### One hand of play
1. **Hole cards:** each player gets 2 private cards, dealt face down.
2. **Preflop:** the first betting round. No community cards are out yet.
3. **Flop:** 3 community cards are dealt face up, followed by a betting round.
4. **Turn:** the 4th community card, followed by a betting round.
5. **River:** the 5th and final community card, followed by the last betting round.
6. **Showdown:** the players who are still in reveal their cards, and the best hand wins the pot.

Each of these stages (preflop, flop, turn, river) is called a **street**. The community cards are
called the **board**, and they're shared by everyone.

### How a player's hand is formed
- Each player has 7 cards available: 2 hole cards + 5 board cards.
- **The hand is the best 5-card combination of those 7.** A player can use 2, 1, or 0 hole cards.
- When a player uses 0 hole cards, the **board plays**. If the board is the best hand for everyone
  still in, they all tie.

### Winning and ties
- The best 5-card hand wins.
- If two or more players have exactly equal 5-card hands, they **split (chop)** the pot equally.
- Betting, folding, and blinds don't matter for this project. We only care about who wins at
  showdown, given which cards are known.

> **Why "river"?** Before the river, the outcome is uncertain and you can only compute
> probabilities. After the river, the outcome is a fact. The equity calculator simulates
> every possible path to the river.

---

## Part 2: Hand rankings and tie-breaking (the evaluator's spec)

From best to worst:

| # | Category | Example | Description |
|---|---|---|---|
| 8 | Straight flush | `9h 8h 7h 6h 5h` | 5 in a row, all the same suit. The A-high one is a "royal flush". |
| 7 | Four of a kind (quads) | `Kc Kd Kh Ks 3d` | 4 of one rank |
| 6 | Full house | `Qs Qh Qd 4c 4s` | 3 of one rank + 2 of another |
| 5 | Flush | `Ad Jd 9d 6d 2d` | 5 of the same suit, not in sequence |
| 4 | Straight | `Tc 9d 8h 7s 6c` | 5 in a row, mixed suits |
| 3 | Three of a kind (trips/set) | `7c 7d 7h Ks 2d` | 3 of one rank |
| 2 | Two pair | `Jc Jd 5h 5s Ac` | 2 + 2 |
| 1 | One pair | `Tc Th Ad 8s 4c` | 2 of one rank |
| 0 | High card | `Ac Jd 8h 6s 3c` | Nothing else |

### Tie-breaking within a category

Compare the following in order. The first difference decides the winner.

| Category | Compare |
|---|---|
| Straight flush / straight | Highest card only |
| Quads | Quad rank, then 1 kicker |
| Full house | Trips rank, then pair rank |
| Flush | All 5 cards, highest first |
| Trips | Trips rank, then 2 kickers (highest first) |
| Two pair | High pair, then low pair, then 1 kicker |
| One pair | Pair rank, then 3 kickers |
| High card | All 5 cards, highest first |

A **kicker** is a leftover card that only matters for breaking ties. Only the best 5 cards count, so the 6th and 7th cards never matter.

### Edge cases (put each of these in a unit test)
1. **Wheel:** `A 2 3 4 5` is a straight, and the ace plays **low**. It's the lowest straight (5-high).
2. **No wrap-around:** `Q K A 2 3` is **not** a straight.
3. **The ace is high everywhere else:** `A K Q J T` is the highest straight.
4. **6 or 7 cards in a row:** use the highest 5. For example, `4 5 6 7 8 9` is a 9-high straight.
5. **Three pairs among 7 cards:** use the best two pairs. The third pair's higher card can still be the kicker.
   For example, `KK 99 44 A` plays as `KK 99 A`. But `KK 99 44 2` plays as `KK 99 4`.
6. **Two sets of trips among 7 cards:** this makes a full house, using the higher trips plus a pair from the lower trips.
7. **Trips + two pairs:** a full house with the best available pair.
8. **6 or 7 cards of one suit:** the flush uses the highest 5 cards of that suit.
9. **Straight + flush that aren't the same cards:** that's just a flush, not a straight flush.
   A straight flush requires the same 5 cards to be both suited and consecutive.
10. **Counterfeiting:** board cards can make your hole cards worthless. For example, you hold `22`,
    the board is `AA KK Q`, and you play `AA KK Q`. Anyone holding a Q or worse ties with you.

### Useful fact for the flush fast path
In 7 cards, **if a flush exists, quads and full houses are impossible.** A flush uses 5 of the 7 cards in one
suit, which leaves only 2 other cards. That isn't enough to build 4 of a rank or 3+2. Only one suit can
have 5 or more cards. So when a flush exists, the answer is always "flush or straight flush", and the
lookup evaluator can take a separate path for it.

### Counting the distinct hand values
There are exactly **7,462** distinct 5-card hand strengths:

| Category | Distinct values |
|---|---|
| Straight flush | 10 |
| Quads | 156 (13 quad ranks × 12 kickers) |
| Full house | 156 (13 × 12) |
| Flush | 1,277 (C(13,5) − 10 straights) |
| Straight | 10 |
| Trips | 858 (13 × C(12,2)) |
| Two pair | 858 (C(13,2) × 11) |
| One pair | 2,860 (13 × C(12,3)) |
| High card | 1,277 |

So the evaluator can return a number from 1 to 7462, where a bigger number is a better hand. That
number is the **rank** (or "hand value"). The whole project depends on every evaluator agreeing
on this number.

---

## Part 3: Poker vocabulary for this project

### Hand notation
| Notation | Meaning | Number of combos |
|---|---|---|
| `AhKh` | One specific hand | 1 |
| `AKs` | Ace-king **suited** (same suit) | 4 |
| `AKo` | Ace-king **offsuit** (different suits) | 12 |
| `AK` | Both of the above | 16 |
| `QQ` | Pocket pair of queens | 6 |

### Ranges (M4)
A **range** is the set of hands an opponent could have. Notation:

| Syntax | Expands to |
|---|---|
| `QQ+` | QQ, KK, AA |
| `ATs+` | ATs, AJs, AQs, AKs |
| `A5s-A2s` | A5s, A4s, A3s, A2s |
| `T9s+` | **Connector** rule: T9s, JTs, QJs, KQs (both ranks go up together) |
| `22+` | All pocket pairs |
| `random` | All 1,326 hands |
| `AKo:0.5` | AKo at 50% weight |

### Core terms
- **Equity:** your share of the pot if all the remaining cards were dealt out, averaged over all possible
  runouts. Ties count as a fractional win (1/k for a k-way tie). **This is the main thing the project computes.**
- **Win% / Tie%:** how often you win outright, and how often you chop. Equity = win% + Σ(tie share).
- **Runout:** the remaining board cards that get dealt.
- **Outs:** cards that would turn your hand into the winner. For example, a flush draw with 4 suited cards has 9 outs.
- **Draw:** a hand that isn't winning yet but has outs. Examples: a flush draw, a straight draw (open-ended = 8 outs, gutshot = 4).
- **Dead cards:** cards known to be out of play (folded or exposed). They're removed from the deck.
- **Card removal / blockers:** a card in your hand can't be in anyone else's. If you hold `As`, there are only 3 AA combos left for opponents instead of 6.
- **Nuts:** the best possible hand given the board.
- **Overcard:** a card higher than any card on the board, or higher than an opponent's pair.
- **Set vs trips:** a pocket pair plus one on the board is a **set**. One in the hand plus a pair on the board is **trips**. The evaluator treats both the same; the names only matter to humans.

---

## Part 4: Strategy (context: why equity matters)

The engine doesn't need any of this. It explains what the engine is for, and it gives you test values.

### Pot odds: the main use of equity
When facing a bet, calling is profitable if:

```
equity  >  call_amount / (pot_after_call)
```

Example: the pot is $100, the opponent bets $50, and you must call $50 to win $200 total.
You need equity > 50/200 = **25%**. If the calculator says your draw has 35% equity, calling is profitable.

### Expected value (EV)
`EV = equity × final_pot − cost_to_call`. Good decisions maximize EV. Any single hand is noisy (that's **variance**), but EV holds up over many hands.

### Thinking in ranges
Good players don't put an opponent on one exact hand. They put them on a range, which is why M4 (range vs range)
is how real players actually use a tool like this.

### Classic matchups (approximate preflop equities; use them as sanity tests)
| Matchup | Name | Approximate equity |
|---|---|---|
| AA vs KK | Overpair vs pair | ~82% / 18% |
| Pair vs two overcards (e.g. QQ vs AKs) | "Coin flip" | ~54% / 46% |
| Pair vs two undercards (e.g. JJ vs T9) | | ~80% / 20% |
| AK vs AQ | **Domination** (they share a card and one kicker is better) | ~74% / 26% |
| AA vs 1 random hand | | ~85% |
| AA vs 8 random hands (9 players) | | ~35% |

Lessons from these numbers:
- Even the best hand (AA) loses about 65% of the time in a 9-way pot. This is why the multi-player feature matters.
- Suited hands get about 2–3% extra equity from flush chances.
- Sharing a card with an opponent ("domination") is much worse than facing two different cards.

### Draw strength on the flop (rule of 2 and 4)
A rough rule: `outs × 4 ≈ % to hit by the river` (from the flop), and `outs × 2 ≈ % to hit on the next card`.
For a flush draw, 9 × 4 ≈ 36%; the exact figure is 35.0% (see Part 5). The outs feature in M6 can show both numbers.

---

## Part 5: Combinatorics and probability

### The key tool: combinations
`C(n, k) = n! / (k! (n−k)!)` is the number of ways to choose k items from n when order doesn't matter.
Card dealing is a combination problem: being dealt A then K is the same hand as K then A.

### Numbers to know
| Quantity | Formula | Value |
|---|---|---|
| Starting hands (2 cards) | C(52,2) | **1,326** |
| Strategically distinct starting hands | 13 pairs + 78 suited + 78 offsuit | **169** |
| Combos per pair / suited / offsuit | C(4,2) / 4 / 4×3 | 6 / 4 / 12 |
| Check: 13×6 + 78×4 + 78×12 | | 78 + 312 + 936 = 1,326 ✓ |
| All 5-card hands | C(52,5) | 2,598,960 |
| All 7-card hands | C(52,7) | 133,784,560 |
| Full boards, heads-up, preflop (4 cards known) | C(48,5) | 1,712,304 |
| Flops after 2 hole cards | C(50,3) | 19,600 |
| Turn+river runouts, heads-up, on the flop (7 known) | C(45,2) | 990 |
| Turn+river runouts, 3 players, on the flop (9 known) | C(43,2) | 903 |
| Full boards, 9 known hands, preflop (18 known) | C(34,5) | 278,256 |

### When exact enumeration is too slow
Each unknown opponent hand multiplies the work:
- Heads-up vs 1 random hand preflop: C(50,2) × C(48,5) = 1,225 × 1,712,304 ≈ **2.1 billion**.
  That's possible with a fast evaluator and threads, but it takes seconds.
- Against 8 random hands, the count is astronomically large. You can't enumerate it, so you use **Monte Carlo** (Part 8).

That's why the plan has an **auto mode**: it counts the combinations first, then picks exact enumeration or Monte Carlo.

### Probability of hitting outs
"Hit at least once" is easier to compute as 1 minus "miss every time".

Flush draw on the flop: 9 outs, 47 unseen cards, 2 cards to come.
```
P(miss turn)  = 38/47
P(miss river) = 37/46
P(hit)        = 1 − (38/47)(37/46) = 1 − 1406/2162 ≈ 35.0%
```
For an open-ended straight draw with 8 outs: 1 − (39/47)(38/46) ≈ 31.5%.

"Unseen" means you only remove the cards you know about. Opponents' unknown cards still count as unseen.

### Category frequencies (your exhaustive tests)
See the tables in `PLAN.md` §5. For example, there are 624 quads among the 2,598,960 five-card hands:
13 ranks × 48 kickers = 624. **You can derive most of these by hand.** Doing that is good practice and shows you understand where the test numbers come from.

### Equity formula
For each runout (or sample), find the best rank among the players. If k players share it, each of them gets `1/k`.
```
equity_i = (Σ over runouts of share_i) / number_of_runouts
```
With exact enumeration, every runout is equally likely, so this is a plain average. Card removal is
handled automatically because you only deal from cards that aren't already used.

---

## Part 6: C and bit-manipulation basics

### Types
- `uint64_t` (from `<stdint.h>`): an unsigned 64-bit integer, used as the card bitmask.
- `uint16_t`: the hand rank (it only needs to hold up to 7,462).
- Always use **unsigned** types for bit operations. Shifting signed values can be undefined behavior.

### Bit operators
| Op | Meaning | Card use |
|---|---|---|
| `a \| b` | union | Combine hole cards and the board |
| `a & b` | intersection | Do two hands share a card? |
| `a ^ b` | either one but not both | Parity tricks (counting ranks, Part 7) |
| `~a` | complement | "Cards not used" (`~used & FULL_DECK`) |
| `a << n`, `a >> n` | shift | Line ranks up for straight detection |
| `1ULL << i` | the bit for card i | `ULL` matters: plain `1 << 40` is undefined |

### Built-in operations (each is a single CPU instruction)
- `__builtin_popcountll(x)`: number of set bits, i.e. how many cards (or how many cards of a suit).
- `__builtin_ctzll(x)`: count trailing zeros, i.e. the index of the **lowest** set bit.
- `__builtin_clzll(x)`: count leading zeros. `63 − clz` = the index of the **highest** set bit (the top rank).
- `x & (x − 1)`: clears the lowest set bit. Use it to iterate over cards: `while (x) { i = ctz(x); x &= x-1; }`.

### The card layout used in this project
```
bit:   63..48     47..32     31..16     15..0
       spades     hearts     diamonds   clubs
       ---AKQJT98765432 (13 bits used per 16-bit lane)
```
`suit_ranks(h, s) = (h >> (16*s)) & 0x1FFF` gives a 13-bit mask of the ranks present in suit s.

### Other C items
- Header files and `static inline` functions for small hot functions
- `const` correctness and fixed-size structs (no `malloc` needed in the hot path)
- Build with CMake, and enable compiler warnings
- **Sanitizers:** `-fsanitize=address,undefined` catch out-of-bounds reads and undefined shifts in tests
- Generated source files (lookup tables written out as `.c` arrays)

---

## Part 7: Algorithms

### 7.1 Parsing (M0)
`"AhKd"` → for each 2-character chunk, find the rank in `"23456789TJQKA"` and the suit in `"cdhs"`, then set the bit.
Reject duplicates (`if (hand & bit) error`).

### 7.2 Naive evaluator (M1): the reference oracle
1. Loop over all 21 ways to choose 5 of the 7 cards.
2. For each 5-card hand:
   - Count how many cards of each rank and each suit.
   - Flush? Check whether all 5 cards share a suit. Straight? Check whether there are 5 distinct consecutive ranks, plus the wheel.
   - Sort the ranks by (count desc, rank desc). For example, `KK 9 9 A` sorts as `K K 9 9 A`.
   - Encode as `category << 12 | tiebreak` (or map to 1..7462).
3. Return the max over all 21.

Use it to check everything else. It's slow and simple, so its correctness is easy to trust.

### 7.3 Bitwise evaluator (M1): direct 7-card evaluation
Read the four 13-bit suit masks `s0..s3`.

```
any      = s0 | s1 | s2 | s3                 // ranks present (≥1)
quads    = s0 & s1 & s2 & s3                 // count == 4
odd      = s0 ^ s1 ^ s2 ^ s3                 // count is 1 or 3
atleast2 = (s0 & s1) | (s2 & s3) | ((s0 | s1) & (s2 | s3))
trips    = odd & atleast2                    // count == 3
pairs    = atleast2 & ~odd & ~quads          // count == 2
singles  = odd & ~atleast2                   // count == 1
```
Then check categories from best to worst:
1. If some suit has popcount ≥ 5, it's a flush. Check that suit's mask for a straight to see if it's a straight flush. Otherwise take its top 5 bits.
2. Quads → quad rank + the best other card as the kicker.
3. Trips plus another trips or a pair → full house.
4. Straight check on `any`:
   ```
   m = (any << 1) | (any >> 12 & 1)        // copy the ace below the 2 for the wheel
   run = m & (m >> 1) & (m >> 2) & (m >> 3) & (m >> 4)
   if (run) top_rank = highest_bit(run) + 3  // run bit i means m bits i..i+4 are set; m bit j is rank j-1
   ```
5. Trips, two pair, pair, and high card come next. Pick kickers by repeatedly taking the highest bit with `clz`.

It needs no tables, and every operation is a few CPU instructions.

### 7.4 Perfect-hash lookup evaluator (M2)
The idea is to do all the work once ahead of time, store the answers in tables, and make each lookup a single array read.

**Flush path:** a table indexed directly by the 13-bit suit mask (2^13 = 8,192 entries). Each entry holds the best
flush or straight-flush rank for that set of ranks. Per the Part 2 fact, nothing can beat it.

**Non-flush path:** suits no longer matter, only **how many of each rank** you have (the *rank multiset*).
There are 49,205 possible 7-card rank multisets. That number is much smaller than 133M, but you need a
way to turn a multiset into a table index:
- **Option A, prime product** (the classic "Cactus Kev" idea): give each rank a prime (2,3,5,7,…,41) and multiply the primes for your 7 cards.
  Prime factorization is unique, so each multiset gets a unique product. The products are huge and sparse, though, so you still need a hash.
- **Option B, weighted sum:** choose a weight per rank so that every 7-card multiset gets a distinct sum.
  The generator searches for such weights, then you shrink the table with a modulus or a second-level table.
- **Perfect hash:** a hash function with **zero collisions** on a known, fixed key set. **Minimal** means
  the table size equals the number of keys. A standard method is **hash-and-displace (CHD)**: hash keys into buckets, then for each bucket
  search for a displacement value that puts all its keys into empty slots.

`tools/gen_tables.c` builds every multiset, scores it with the naive evaluator, builds the hash,
**asserts there are no collisions**, and writes the tables to a `.c` file.

To keep a running key cheaply, give each card a precomputed value and **add** card values to form the key.
That makes adding a card O(1), which helps a lot in enumeration loops.

### 7.5 Exact enumeration (M1, M3)
- **Nested loops** over the remaining deck: `for a < b < c < d < e` choose 5 board cards from those left.
  Keeping the indices increasing means each combination is visited exactly once.
- **Gosper's hack** (optional) moves to the next integer with the same number of set bits, which enumerates k-subsets directly as bitmasks.
- **Incremental evaluation (M5):** compute the board's contribution once per board, then add each player's 2 cards. Don't redo the board 9 times.

### 7.6 Monte Carlo (M3)
1. Build an array of the unused cards.
2. **Partial Fisher–Yates shuffle:** for i = 0..k−1, swap card i with a random card j in [i, n). The first k cards are now a uniform random sample.
   Deal the missing board cards and the random opponents' hole cards from them.
3. Evaluate, add up the shares, and repeat N times.
4. **Avoid modulo bias:** `rand64() % n` slightly favors small values. Use Lemire's multiply-shift method or rejection sampling.

### 7.7 PRNG
**xoshiro256\*\***: 256 bits of state, very fast, and high quality. Seed it with **SplitMix64**.
For threads, give each thread its own stream (`jump()` function, or a separate seed per thread).
`rand()` is too slow, low quality, and not thread-safe.

### 7.8 Range parsing (M4)
Tokenize on commas. Each token is a hand class with optional `+`, `-`, or `:weight`. Expand it to a list of
specific combos (bitmasks), then drop any combo that overlaps known/dead cards.
Range vs range is tricky because the two players can't hold the same card. Sample one player's combo, then
pick a compatible combo for the other.

### 7.9 Parallelism (M5)
Enumeration and Monte Carlo are **embarrassingly parallel**: the samples don't depend on each other. Split the work
across pthreads, give each thread private counters, and sum them at the end. Shared counters would add contention and false sharing.

---

## Part 8: Statistics for Monte Carlo (M3)

A Monte Carlo equity is an **estimate**. Report how precise it is.

- **Standard error:** `SE = sqrt(p(1 − p) / N)`, where p is the estimated equity and N is the number of samples.
- **95% confidence interval:** `p ± 1.96 × SE`.
- Example: p = 0.5 with N = 1,000,000 gives SE = 0.0005, so the CI is **±0.098%**.
- **Precision improves with √N.** 4× more samples only halves the error.
- **Ties:** with fractional shares, use the sample variance of the per-sample share, not the p(1−p) formula.
  Track `Σx` and `Σx²` so you can compute the variance at the end.
- **Early stopping:** stop when the largest CI across players drops below a target such as ±0.1%.
- **Testing MC:** with a fixed seed, check that |MC − exact| ≤ about 3 SE. A seeded run is deterministic, so the test can't be flaky.

---

## Part 9: Performance and benchmarking (M2, M5)

- **Throughput:** evaluations per second. Use the same input set for every evaluator.
- **Sequential vs random input order:** sequential input is cache-friendly and makes lookup tables look better than
  they are. Random order shows real behavior. Report both.
- **Dead-code elimination:** the compiler deletes work whose result is never used. Accumulate the results
  (`sum += eval(h)`) and print the sum.
- **Warm-up:** run once before timing so caches and the CPU clock speed settle.
- **Median of runs:** timings are noisy, and the median holds up better than the mean.
- **Cache:** L1 is about 32–64 KB and L2 about 256 KB–1 MB. Tables that fit in L1 or L2 are fast. A smaller table can beat a "faster" algorithm.
- **Branch prediction:** unpredictable `if`s cost about 15 cycles each when mispredicted. Bitwise code with few branches avoids this.
- **Profiling:** use `perf record`/`perf report` (Linux) or Instruments (macOS) to find the hot spots instead of guessing.
- **Compiler flags:** `-O3 -march=native` for benchmarks. Always state the flags and CPU next to the numbers.

---

## Part 10: Glossary

| Term | Meaning |
|---|---|
| Board | The community cards (0–5) |
| Blockers | Cards you hold that reduce which hands opponents can have |
| Chop / split | Tie. The pot is divided equally. |
| Combo | One specific 2-card hand (e.g. `AsKs`) |
| Connector | Consecutive ranks (e.g. `T9`) |
| Counterfeit | When the board makes your hand's advantage worthless |
| Dead cards | Cards known to be out of the deck |
| Domination | Sharing a card with an opponent who has a better kicker |
| Draw | A hand that needs more cards to improve |
| EV | Expected value: the average result of a decision |
| Equity | Expected share of the pot over all runouts |
| Flop / Turn / River | Board cards 3 / 4 / 5 |
| Hole cards | A player's 2 private cards |
| Kicker | A card that only breaks ties |
| Nuts | The best possible hand on this board |
| Offsuit (`o`) | Two cards of different suits |
| Outs | Cards that make your hand the winner |
| Pot odds | The equity needed to call profitably |
| Range | The set of hands a player could have |
| Runout | The cards that are still to be dealt |
| Showdown | When the remaining hands are revealed and compared |
| Street | A betting stage: preflop / flop / turn / river |
| Suited (`s`) | Two cards of the same suit |
| Wheel | A-2-3-4-5, the lowest straight |
| Perfect hash | A collision-free hash on a fixed key set |
| Monte Carlo | Estimating by random sampling |
| Fisher–Yates | An unbiased shuffle algorithm |
| popcount / ctz / clz | Bit count / lowest set bit / highest set bit |

---

## Study order (checklist)

- [ ] **Before M0:** Part 1, Part 6 (bit ops and the card layout)
- [ ] **Before M1:** Part 2 (all the edge cases), Part 5 (C(n,k) and the frequency tables), Part 7.1–7.3 and 7.5
- [ ] **Before M2:** Part 7.4, Part 9
- [ ] **Before M3:** Part 5 (when enumeration is too slow), Part 7.6–7.7, Part 8
- [ ] **Before M4:** Part 3 (ranges), Part 7.8
- [ ] **Before M5:** Part 7.9, Part 9 (profiling and cache)
- [ ] **Before M6:** Part 4 (so the UI shows things players care about: equity by street, outs, pot odds)
