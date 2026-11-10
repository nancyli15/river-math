# RiverMath

![CI](https://github.com/nancyli15/river-math/actions/workflows/ci.yml/badge.svg)

A fast Texas Hold'em hand evaluator and multi-player equity calculator in C.

> Status: **M0 (scaffolding)**. Card parsing and the bitmask representation are done.
> The evaluators and equity calculator are coming. See [PLAN.md](PLAN.md) for the roadmap and
> [CONCEPTS.md](CONCEPTS.md) for the poker, math and algorithms background.

## Build

Requires a C17 compiler (clang or gcc) and `make`. There are no other dependencies.

```sh
make                         # release build -> build/release/rivermath
make test                    # debug build with sanitizers, runs all tests
make run ARGS="cards AhKd"   # build and run the CLI
make clean
```

## Usage

```
$ ./build/release/rivermath cards Ah Kd 2c
Cards:   AhKd2c
Count:   3
Bitmask: 0x0000100008000001
```

Cards are written as rank + suit: ranks `23456789TJQKA`, suits `c d h s`.

## Layout

```
include/rivermath/   public headers (card.h, bits.h, ...)
src/                 library sources
apps/cli/            the rivermath command-line tool
tests/               unit tests
```
