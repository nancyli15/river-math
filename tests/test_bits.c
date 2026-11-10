#include "rivermath/bits.h"
#include "test.h"

static void test_popcount(void) {
    CHECK_EQ(rm_popcount64(0), 0);
    CHECK_EQ(rm_popcount64(1), 1);
    CHECK_EQ(rm_popcount64(0xFF), 8);
    CHECK_EQ(rm_popcount64(~0ULL), 64);
    CHECK_EQ(rm_popcount64(1ULL << 63), 1);
}

static void test_ctz_clz(void) {
    CHECK_EQ(rm_ctz64(1), 0);
    CHECK_EQ(rm_ctz64(0x8), 3);
    CHECK_EQ(rm_ctz64(1ULL << 63), 63);
    CHECK_EQ(rm_clz64(1), 63);
    CHECK_EQ(rm_clz64(1ULL << 63), 0);
}

static void test_highest_bit(void) {
    CHECK_EQ(rm_highest_bit(0), -1);
    CHECK_EQ(rm_highest_bit(1), 0);
    CHECK_EQ(rm_highest_bit(0x1FFF), 12);
    CHECK_EQ(rm_highest_bit(~0ULL), 63);
}

static void test_clear_lowest(void) {
    CHECK_EQ(rm_clear_lowest(0), 0);
    CHECK_EQ(rm_clear_lowest(0xC), 0x8);
    CHECK_EQ(rm_clear_lowest(1ULL << 63), 0);
}

void suite_bits(void) {
    RUN_TEST(test_popcount);
    RUN_TEST(test_ctz_clz);
    RUN_TEST(test_highest_bit);
    RUN_TEST(test_clear_lowest);
}
