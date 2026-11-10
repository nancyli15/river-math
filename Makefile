# RiverMath build
#
#   make              release build of the CLI      -> build/release/rivermath
#   make test         debug build + sanitizers, run the test suite
#   make test-full    release build, also check every 7-card hand (~1 min)
#   make run ARGS=".." build and run the CLI, e.g. make run ARGS="cards AhKd"
#   make clean
#
# BUILD=release (default) or BUILD=debug selects flags and output directory.

CC     ?= cc
BUILD  ?= release

CSTD     := -std=c17
WARNINGS := -Wall -Wextra -Wpedantic -Werror -Wshadow -Wstrict-prototypes \
            -Wmissing-prototypes -Wundef
CPPFLAGS := -Iinclude -MMD -MP

ifeq ($(BUILD),debug)
  OPT      := -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined \
              -fno-sanitize-recover=all
  LDFLAGS  += -fsanitize=address,undefined
else ifeq ($(BUILD),release)
  OPT      := -O3 -DNDEBUG
else
  $(error BUILD must be 'release' or 'debug')
endif

CFLAGS  += $(CSTD) $(WARNINGS) $(OPT)
OUT     := build/$(BUILD)

LIB_SRC  := $(wildcard src/*.c)
CLI_SRC  := $(wildcard apps/cli/*.c)
TEST_SRC := $(wildcard tests/*.c)

LIB_OBJ  := $(LIB_SRC:%.c=$(OUT)/%.o)
CLI_OBJ  := $(CLI_SRC:%.c=$(OUT)/%.o)
TEST_OBJ := $(TEST_SRC:%.c=$(OUT)/%.o)

LIB      := $(OUT)/librivermath.a
CLI      := $(OUT)/rivermath
TESTS    := $(OUT)/rivermath_tests

.PHONY: all cli test test-full run clean

all: cli

cli: $(CLI)

# Tests always build in debug mode so sanitizers are on.
test:
	@$(MAKE) --no-print-directory BUILD=debug build/debug/rivermath_tests
	./build/debug/rivermath_tests

# The exhaustive 133M-hand checks are too slow under sanitizers, so use the release build.
test-full:
	@$(MAKE) --no-print-directory BUILD=release build/release/rivermath_tests
	./build/release/rivermath_tests --full

run: $(CLI)
	./$(CLI) $(ARGS)

$(LIB): $(LIB_OBJ)
	@mkdir -p $(@D)
	$(AR) rcs $@ $^

$(CLI): $(CLI_OBJ) $(LIB)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

$(TESTS): $(TEST_OBJ) $(LIB)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

$(OUT)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build

-include $(LIB_OBJ:.o=.d) $(CLI_OBJ:.o=.d) $(TEST_OBJ:.o=.d)
