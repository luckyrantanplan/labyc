/**
 * @file test_queuecost.cpp
 * @brief Unit tests for QueueCost comparison and ordering.
 */

#include "Anisotrop/QueueCost.h"
#include <gtest/gtest.h>
#include <sstream>
#include <string>

using laby::aniso::QueueCost;

namespace {

constexpr int32_t kDefaultDistance = -1;
constexpr int32_t kDefaultValue = 0;
constexpr int32_t kCongestionLow = 1;
constexpr int32_t kCongestionMid = 2;
constexpr int32_t kCongestionHigh = 3;
constexpr int32_t kViaLow = 2;
constexpr int32_t kViaHigh = 5;
constexpr int32_t kDistanceLow = 10;
constexpr int32_t kDistanceHigh = 20;
constexpr int32_t kRandomLow = 3;
constexpr int32_t kRandomHigh = 7;
constexpr int32_t kPrintedDistance = 100;
constexpr int32_t kMemoryValueOne = 1;
constexpr int32_t kMemoryValueTwo = 2;
constexpr int32_t kMemoryValueThree = 3;
constexpr int32_t kMemoryValueFour = 4;
constexpr int32_t kMemoryValueFive = 5;

} // namespace

TEST(QueueCostTest, DefaultValues) {
    QueueCost const queueCost;
    EXPECT_EQ(queueCost.distance(), kDefaultDistance);
    EXPECT_EQ(queueCost.viaNum(), kDefaultValue);
    EXPECT_EQ(queueCost.congestion(), kDefaultValue);
    EXPECT_EQ(queueCost.randomization(), kDefaultValue);
}

TEST(QueueCostTest, LessThanByCongestion) {
    QueueCost lowerCost;
    lowerCost.congestion() = kCongestionLow;
    QueueCost higherCost;
    higherCost.congestion() = kCongestionMid;
    EXPECT_TRUE(lowerCost < higherCost);
    EXPECT_FALSE(higherCost < lowerCost);
}

TEST(QueueCostTest, GreaterThanByCongestion) {
    QueueCost higherCost;
    higherCost.congestion() = kCongestionHigh;
    QueueCost lowerCost;
    lowerCost.congestion() = kCongestionLow;
    EXPECT_TRUE(higherCost > lowerCost);
    EXPECT_FALSE(lowerCost > higherCost);
}

TEST(QueueCostTest, TieBreakByVia) {
    QueueCost lowerCost;
    lowerCost.congestion() = kCongestionLow;
    lowerCost.viaNum() = kViaLow;
    QueueCost higherCost;
    higherCost.congestion() = kCongestionLow;
    higherCost.viaNum() = kViaHigh;
    EXPECT_TRUE(lowerCost < higherCost);
    EXPECT_FALSE(lowerCost > higherCost);
}

TEST(QueueCostTest, TieBreakByDistance) {
    QueueCost lowerCost;
    lowerCost.congestion() = kCongestionLow;
    lowerCost.viaNum() = kViaLow;
    lowerCost.distance() = kDistanceLow;
    QueueCost higherCost;
    higherCost.congestion() = kCongestionLow;
    higherCost.viaNum() = kViaLow;
    higherCost.distance() = kDistanceHigh;
    EXPECT_TRUE(lowerCost < higherCost);
    EXPECT_FALSE(lowerCost > higherCost);
}

TEST(QueueCostTest, TieBreakByRandomization) {
    QueueCost lowerCost;
    lowerCost.congestion() = kCongestionLow;
    lowerCost.viaNum() = kViaLow;
    lowerCost.distance() = kDistanceLow;
    lowerCost.randomization() = kRandomLow;
    QueueCost higherCost;
    higherCost.congestion() = kCongestionLow;
    higherCost.viaNum() = kViaLow;
    higherCost.distance() = kDistanceLow;
    higherCost.randomization() = kRandomHigh;
    EXPECT_TRUE(lowerCost < higherCost);
    EXPECT_FALSE(lowerCost > higherCost);
}

TEST(QueueCostTest, EqualCostsNeitherLessNorGreater) {
    QueueCost firstCost;
    firstCost.congestion() = kCongestionLow;
    firstCost.viaNum() = kViaLow;
    firstCost.distance() = kDistanceLow;
    QueueCost secondCost;
    secondCost.congestion() = kCongestionLow;
    secondCost.viaNum() = kViaLow;
    secondCost.distance() = kDistanceLow;
    EXPECT_FALSE(firstCost < secondCost);
    EXPECT_FALSE(firstCost > secondCost);
}

TEST(QueueCostTest, PrintOutput) {
    QueueCost queueCost;
    queueCost.congestion() = kViaHigh;
    queueCost.viaNum() = kRandomLow;
    queueCost.distance() = kPrintedDistance;
    queueCost.randomization() = kRandomHigh;
    std::ostringstream outputStream;
    queueCost.print(outputStream);
    std::string const output = outputStream.str();
    EXPECT_NE(output.find('5'), std::string::npos);
    EXPECT_NE(output.find('3'), std::string::npos);
    EXPECT_NE(output.find("100"), std::string::npos);
    EXPECT_NE(output.find('7'), std::string::npos);
}

TEST(QueueCostTest, MemorySetsIndependent) {
    QueueCost queueCost;
    queueCost.memorySource().insert(kMemoryValueOne);
    queueCost.memorySource().insert(kMemoryValueTwo);
    queueCost.futureMemorySource().insert(kMemoryValueThree);
    queueCost.memoryTarget().insert(kMemoryValueFour);
    queueCost.futureMemoryTarget().insert(kMemoryValueFive);

    EXPECT_EQ(queueCost.memorySource().size(), 2U);
    EXPECT_EQ(queueCost.futureMemorySource().size(), 1U);
    EXPECT_EQ(queueCost.memoryTarget().size(), 1U);
    EXPECT_EQ(queueCost.futureMemoryTarget().size(), 1U);
}
