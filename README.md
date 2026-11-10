# RiverMath

![CI](https://github.com/nancyli15/river-math/actions/workflows/ci.yml/badge.svg)

A fast Texas Hold'em hand evaluator and multi-player equity calculator in C.

> Status: **M2 done**. Three evaluators (naive, bitwise, perfect-hash lookup) agree on all
> 133,784,560 seven-card hands. The equity calculator is coming. See [PLAN.md](PLAN.md) for the roadmap and
> [CONCEPTS.md](CONCEPTS.md) for the poker, math and algorithms background.

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
```

Cards are written as rank + suit: ranks `23456789TJQKA`, suits `c d h s`.

## Layout

```
include/rivermath/   public headers (card.h, bits.h, ...)
src/                 library sources
apps/cli/            the rivermath command-line tool
tests/               unit tests
```
