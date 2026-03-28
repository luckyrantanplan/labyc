/**
 * @file test_randomunidist.cpp
 * @brief Unit tests for RandomUniDist.
 */

#include "basic/RandomUniDist.h"
#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>
#include <numeric>
#include <vector>

using laby::basic::RandomUniDist;

namespace {

constexpr double kUnitRangeMin = 0.0;
constexpr double kUnitRangeMax = 1.0;
constexpr double kWideRangeMax = 10.0;
constexpr double kCustomRangeMin = -5.0;
constexpr double kCustomRangeMax = 5.0;
constexpr int kDefaultSeed = 42;
constexpr int kSharedSeed = 123;
constexpr int kFirstSeed = 1;
constexpr int kSecondSeed = 2;
constexpr int kRangeCheckIterations = 1000;
constexpr int kDeterminismIterations = 100;
constexpr int kDifferenceIterations = 10;
constexpr int kCustomRangeIterations = 100;
constexpr unsigned int kSelectionMin = 0U;
constexpr unsigned int kSelectionMax = 10U;
constexpr unsigned int kSingleSelectionValue = 5U;
constexpr unsigned int kSingleSelectionEnd = 6U;
constexpr double kDifferenceTolerance = 1e-12;
constexpr int kShufflePreservedCount = 10;
constexpr int kShuffleChangedCount = 15;

auto makeSequentialValues(int count) -> std::vector<int> {
    std::vector<int> values(static_cast<std::size_t>(count));
    std::iota(values.begin(), values.end(), 1);
    return values;
}

} // namespace

TEST(RandomUniDistTest, ValueInRange) {
    RandomUniDist randomGenerator(kUnitRangeMin, kUnitRangeMax, kDefaultSeed);
    for (int iteration = 0; iteration < kRangeCheckIterations; ++iteration) {
        double const generatedValue = randomGenerator.get();
        EXPECT_GE(generatedValue, kUnitRangeMin);
        EXPECT_LE(generatedValue, kUnitRangeMax);
    }
}

TEST(RandomUniDistTest, DeterministicWithSameSeed) {
    RandomUniDist firstGenerator(kUnitRangeMin, kWideRangeMax, kSharedSeed);
    RandomUniDist secondGenerator(kUnitRangeMin, kWideRangeMax, kSharedSeed);
    for (int iteration = 0; iteration < kDeterminismIterations; ++iteration) {
        EXPECT_DOUBLE_EQ(firstGenerator.get(), secondGenerator.get());
    }
}

TEST(RandomUniDistTest, DifferentSeedsDifferentSequence) {
    RandomUniDist firstGenerator(kUnitRangeMin, kUnitRangeMax, kFirstSeed);
    RandomUniDist secondGenerator(kUnitRangeMin, kUnitRangeMax, kSecondSeed);
    bool sequencesMatch = true;
    for (int iteration = 0; iteration < kDifferenceIterations; ++iteration) {
        if (std::abs(firstGenerator.get() - secondGenerator.get()) > kDifferenceTolerance) {
            sequencesMatch = false;
            break;
        }
    }
    EXPECT_FALSE(sequencesMatch);
}

TEST(RandomUniDistTest, CustomRange) {
    RandomUniDist randomGenerator(kCustomRangeMin, kCustomRangeMax, kDefaultSeed);
    for (int iteration = 0; iteration < kCustomRangeIterations; ++iteration) {
        double const generatedValue = randomGenerator.get();
        EXPECT_GE(generatedValue, kCustomRangeMin);
        EXPECT_LE(generatedValue, kCustomRangeMax);
    }
}

TEST(RandomUniDistTest, SelectInRange) {
    RandomUniDist randomGenerator(kUnitRangeMin, kUnitRangeMax, kDefaultSeed);
    for (int iteration = 0; iteration < kCustomRangeIterations; ++iteration) {
        auto const selectedIndex = randomGenerator.select(kSelectionMin, kSelectionMax);
        EXPECT_GE(selectedIndex, kSelectionMin);
        EXPECT_LT(selectedIndex, kSelectionMax);
    }
}

TEST(RandomUniDistTest, SelectSingleElement) {
    RandomUniDist randomGenerator(kUnitRangeMin, kUnitRangeMax, kDefaultSeed);
    auto const selectedIndex = randomGenerator.select(kSingleSelectionValue, kSingleSelectionEnd);
    EXPECT_EQ(selectedIndex, kSingleSelectionValue);
}

TEST(RandomUniDistTest, ShufflePreservesElements) {
    RandomUniDist randomGenerator(kUnitRangeMin, kUnitRangeMax, kDefaultSeed);
    std::vector<int> const original = makeSequentialValues(kShufflePreservedCount);
    std::vector<int> shuffled = original;
    randomGenerator.shuffle(shuffled);

    std::sort(shuffled.begin(), shuffled.end());
    EXPECT_EQ(original, shuffled);
}

TEST(RandomUniDistTest, ShuffleChangesOrder) {
    RandomUniDist randomGenerator(kUnitRangeMin, kUnitRangeMax, kDefaultSeed);
    std::vector<int> values = makeSequentialValues(kShuffleChangedCount);
    std::vector<int> const original = values;
    randomGenerator.shuffle(values);
    EXPECT_NE(values, original);
}
