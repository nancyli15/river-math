# RiverMath

![CI](https://github.com/nancyli15/river-math/actions/workflows/ci.yml/badge.svg)

A fast Texas Hold'em hand evaluator and multi-player equity calculator in C.

> Status: **v1 done**. Three evaluators (naive, bitwise, perfect-hash lookup) agree on all
> 133,784,560 seven-card hands, and a 2 to 9 player equity calculator uses exact enumeration
> or Monte Carlo.

## Try it

Needs a C compiler and `make` (on a Mac, install the Xcode command line tools with `xcode-select --install`).

```sh
git clone https://github.com/nancyli15/river-math.git && cd river-math
make

./build/release/rivermath eval Kc Kd 9h 9s 4c 4d 2h       # score a hand -> Two pair (K, 9, 4)
./build/release/rivermath equity AhAs KdKc                # who wins? exact, every possible board
./build/release/rivermath equity AhAs random random       # against unknown hands: 1M simulated deals
./build/release/rivermath bench                           # hands per second on your machine
```

Cards are rank + suit: ranks `23456789TJQKA`, suits `c d h s`. So `AhAs` is the ace of hearts and ace of spades.

## Build

Requires a C17 compiler (clang or gcc) and `make`. There are no other dependencies.

```sh
make                         # release build -> build/release/rivermath
make test                    # debug build with sanitizers, runs all tests
make test-full               # release build, also checks every 7-card hand (~1.5 min)
make run ARGS="cards AhKd"   # build and run the CLI
make clean
```

## Benchmark

`rivermath bench` evaluates 1,048,576 random 7-card hands (fixed seed), median of 5 runs, single thread.
Apple M3, Apple clang 21, `-O3`. Lowest of two runs shown:

| Evaluator | Hands/sec | ns/hand | vs naive |
|---|---|---|---|
| naive (best of 21 five-card subsets) | 1.0 M | 1026 | 1x |
| bitwise (no tables) | 68.7 M | 14.6 | 70x |
| lookup (perfect hash, 466 KB of tables) | 371 M | 2.7 | 381x |

The lookup tables are built at startup in about 250 ms.

Equity (same machine, lookup evaluator, table build excluded):

| Spot | Mode | Time |
|---|---|---|
| AA vs KK preflop | exact, 1,712,304 boards | 19 ms |
| 9 known hands preflop | exact, 278,256 boards | 11 ms |
| AA vs 8 random hands | Monte Carlo, 1,000,000 samples | ~330 ms |

## Usage

```
$ ./build/release/rivermath cards Ah Kd 2c
Cards:   AhKd2c
Count:   3
Bitmask: 0x0000100008000001

$ ./build/release/rivermath eval Kc Kd 9h 9s 4c 4d 2h
Hand:   KcKd9h9s4c4d2h
Result: Two pair (K, 9, 4)
Value:  0x2b7200 (bigger is better)

$ ./build/release/rivermath equity AhAs KdKc
Board: (none)   Mode: exact (1712304 boards)   19 ms

Player  Hand      Win      Tie   Equity
  1     AhAs    81.06%   0.38%   81.26%  ████████████████████████
  2     KcKd    18.55%   0.38%   18.74%  ██████
```

Each player is two cards (`AhAs`) or `random`. Options: `--board`, `--dead`, `--samples`, `--seed`.
If every hand is known the answer is exact; any `random` player switches to Monte Carlo.

Cards are written as rank + suit: ranks `23456789TJQKA`, suits `c d h s`.

## Layout

```
include/rivermath/   public headers (card.h, bits.h, ...)
src/                 library sources
apps/cli/            the rivermath command-line tool
tests/               unit tests
```
