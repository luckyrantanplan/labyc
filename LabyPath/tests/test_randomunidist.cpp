/**
 * @file test_randomunidist.cpp
 * @brief Unit tests for RandomUniDist.
 */

#include "basic/RandomUniDist.h"
#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>
#include <vector>

using laby::basic::RandomUniDist;

TEST(RandomUniDistTest, ValueInRange) {
    RandomUniDist rng(0.0, 1.0, 42);
    for (int i = 0; i < 1000; ++i) {
        double const val = rng.get();
        EXPECT_GE(val, 0.0);
        EXPECT_LE(val, 1.0);
    }
}

TEST(RandomUniDistTest, DeterministicWithSameSeed) {
    RandomUniDist rng1(0.0, 10.0, 123);
    RandomUniDist rng2(0.0, 10.0, 123);
    for (int i = 0; i < 100; ++i) {
        EXPECT_DOUBLE_EQ(rng1.get(), rng2.get());
    }
}

TEST(RandomUniDistTest, DifferentSeedsDifferentSequence) {
    RandomUniDist rng1(0.0, 1.0, 1);
    RandomUniDist rng2(0.0, 1.0, 2);
    bool allEqual = true;
    for (int i = 0; i < 10; ++i) {
        if (std::abs(rng1.get() - rng2.get()) > 1e-12) {
            allEqual = false;
            break;
        }
    }
    EXPECT_FALSE(allEqual);
}

TEST(RandomUniDistTest, CustomRange) {
    RandomUniDist rng(-5.0, 5.0, 42);
    for (int i = 0; i < 100; ++i) {
        double const val = rng.get();
        EXPECT_GE(val, -5.0);
        EXPECT_LE(val, 5.0);
    }
}

TEST(RandomUniDistTest, SelectInRange) {
    RandomUniDist rng(0.0, 1.0, 42);
    for (int i = 0; i < 100; ++i) {
        auto val = rng.select(0, 10);
        EXPECT_GE(val, 0U);
        EXPECT_LT(val, 10U);
    }
}

TEST(RandomUniDistTest, SelectSingleElement) {
    RandomUniDist rng(0.0, 1.0, 42);
    auto val = rng.select(5, 6);
    EXPECT_EQ(val, 5U);
}

TEST(RandomUniDistTest, ShufflePreservesElements) {
    RandomUniDist rng(0.0, 1.0, 42);
    std::vector<int> const original = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<int> shuffled = original;
    rng.shuffle(shuffled);

    std::sort(shuffled.begin(), shuffled.end());
    EXPECT_EQ(original, shuffled);
}

TEST(RandomUniDistTest, ShuffleChangesOrder) {
    RandomUniDist rng(0.0, 1.0, 42);
    std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    std::vector<int> const original = v;
    rng.shuffle(v);
    EXPECT_NE(v, original);
}
