/**
 * @file test_randominteger.cpp
 * @brief Unit tests for RandomInteger template class.
 */

#include "basic/RandomInteger.h"
#include <gtest/gtest.h>
#include <set>

using laby::basic::RandomInteger;

TEST(RandomIntegerTest, ValueInRange) {
    RandomInteger<int> rng(0, 100, 42);
    for (int i = 0; i < 1000; ++i) {
        int val = rng.get();
        EXPECT_GE(val, 0);
        EXPECT_LE(val, 100);
    }
}

TEST(RandomIntegerTest, DeterministicWithSameSeed) {
    RandomInteger<int> rng1(0, 1000, 123);
    RandomInteger<int> rng2(0, 1000, 123);
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(rng1.get(), rng2.get());
    }
}

TEST(RandomIntegerTest, DifferentSeedsDifferentSequence) {
    RandomInteger<int> rng1(0, 1000000, 1);
    RandomInteger<int> rng2(0, 1000000, 2);
    bool all_equal = true;
    for (int i = 0; i < 100; ++i) {
        if (rng1.get() != rng2.get()) {
            all_equal = false;
            break;
        }
    }
    EXPECT_FALSE(all_equal);
}

TEST(RandomIntegerTest, NarrowRange) {
    RandomInteger<int> rng(5, 5, 42);
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(rng.get(), 5);
    }
}

TEST(RandomIntegerTest, NegativeRange) {
    RandomInteger<int> rng(-10, -1, 42);
    for (int i = 0; i < 100; ++i) {
        int val = rng.get();
        EXPECT_GE(val, -10);
        EXPECT_LE(val, -1);
    }
}

TEST(RandomIntegerTest, ProducesVariety) {
    RandomInteger<int> rng(0, 100, 42);
    std::set<int> values;
    for (int i = 0; i < 200; ++i) {
        values.insert(rng.get());
    }
    EXPECT_GT(values.size(), 10U);
}

TEST(RandomIntegerTest, Int64Type) {
    RandomInteger<int64_t> rng(0, 1000000000LL, 42);
    int64_t val = rng.get();
    EXPECT_GE(val, 0LL);
    EXPECT_LE(val, 1000000000LL);
}
