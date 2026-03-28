/**
 * @file test_randominteger.cpp
 * @brief Unit tests for RandomInteger template class.
 */

#include "basic/RandomInteger.h"
#include <cstdint>
#include <gtest/gtest.h>
#include <set>

using laby::basic::RandomInteger;

namespace {

constexpr int kRangeMin = 0;
constexpr int kRangeMax = 100;
constexpr int kSeedPrimary = 42;
constexpr int kWideRangeMax = 1000;
constexpr int kSeedRepeatable = 123;
constexpr int kLargeRangeMax = 1000000;
constexpr int kSeedFirst = 1;
constexpr int kSeedSecond = 2;
constexpr int kIterationCountShort = 10;
constexpr int kIterationCountMedium = 100;
constexpr int kIterationCountLong = 200;
constexpr int kIterationCountWide = 1000;
constexpr int kNarrowRangeValue = 5;
constexpr int kNegativeRangeMin = -10;
constexpr int kNegativeRangeMax = -1;
constexpr std::size_t kMinimumVarietyCount = 10U;
constexpr int64_t kInt64RangeMax = 1000000000LL;

} // namespace

TEST(RandomIntegerTest, ValueInRange) {
    RandomInteger<int> randomGenerator(kRangeMin, kRangeMax, kSeedPrimary);
    for (int iteration = 0; iteration < kIterationCountWide; ++iteration) {
        int const generatedValue = randomGenerator.get();
        EXPECT_GE(generatedValue, kRangeMin);
        EXPECT_LE(generatedValue, kRangeMax);
    }
}

TEST(RandomIntegerTest, DeterministicWithSameSeed) {
    RandomInteger<int> firstGenerator(kRangeMin, kWideRangeMax, kSeedRepeatable);
    RandomInteger<int> secondGenerator(kRangeMin, kWideRangeMax, kSeedRepeatable);
    for (int iteration = 0; iteration < kIterationCountMedium; ++iteration) {
        EXPECT_EQ(firstGenerator.get(), secondGenerator.get());
    }
}

TEST(RandomIntegerTest, DifferentSeedsDifferentSequence) {
    RandomInteger<int> firstGenerator(kRangeMin, kLargeRangeMax, kSeedFirst);
    RandomInteger<int> secondGenerator(kRangeMin, kLargeRangeMax, kSeedSecond);
    bool sequencesMatch = true;
    for (int iteration = 0; iteration < kIterationCountMedium; ++iteration) {
        if (firstGenerator.get() != secondGenerator.get()) {
            sequencesMatch = false;
            break;
        }
    }
    EXPECT_FALSE(sequencesMatch);
}

TEST(RandomIntegerTest, NarrowRange) {
    RandomInteger<int> randomGenerator(kNarrowRangeValue, kNarrowRangeValue, kSeedPrimary);
    for (int iteration = 0; iteration < kIterationCountShort; ++iteration) {
        EXPECT_EQ(randomGenerator.get(), kNarrowRangeValue);
    }
}

TEST(RandomIntegerTest, NegativeRange) {
    RandomInteger<int> randomGenerator(kNegativeRangeMin, kNegativeRangeMax, kSeedPrimary);
    for (int iteration = 0; iteration < kIterationCountMedium; ++iteration) {
        int const generatedValue = randomGenerator.get();
        EXPECT_GE(generatedValue, kNegativeRangeMin);
        EXPECT_LE(generatedValue, kNegativeRangeMax);
    }
}

TEST(RandomIntegerTest, ProducesVariety) {
    RandomInteger<int> randomGenerator(kRangeMin, kRangeMax, kSeedPrimary);
    std::set<int> values;
    for (int iteration = 0; iteration < kIterationCountLong; ++iteration) {
        values.insert(randomGenerator.get());
    }
    EXPECT_GT(values.size(), kMinimumVarietyCount);
}

TEST(RandomIntegerTest, Int64Type) {
    RandomInteger<int64_t> randomGenerator(kRangeMin, kInt64RangeMax, kSeedPrimary);
    int64_t const generatedValue = randomGenerator.get();
    EXPECT_GE(generatedValue, 0LL);
    EXPECT_LE(generatedValue, kInt64RangeMax);
}
