/**
 * @file test_queuecost.cpp
 * @brief Unit tests for QueueCost comparison and ordering.
 */

#include "Anisotrop/QueueCost.h"
#include <gtest/gtest.h>
#include <sstream>
#include <string>

using laby::aniso::QueueCost;

TEST(QueueCostTest, DefaultValues) {
    QueueCost const c;
    EXPECT_EQ(c.distance(), -1);
    EXPECT_EQ(c.viaNum(), 0);
    EXPECT_EQ(c.congestion(), 0);
    EXPECT_EQ(c.randomization(), 0);
}

TEST(QueueCostTest, LessThanByCongestion) {
    QueueCost a;
    a.congestion() = 1;
    QueueCost b;
    b.congestion() = 2;
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
}

TEST(QueueCostTest, GreaterThanByCongestion) {
    QueueCost a;
    a.congestion() = 3;
    QueueCost b;
    b.congestion() = 1;
    EXPECT_TRUE(a > b);
    EXPECT_FALSE(b > a);
}

TEST(QueueCostTest, TieBreakByVia) {
    QueueCost a;
    a.congestion() = 1;
    a.viaNum() = 2;
    QueueCost b;
    b.congestion() = 1;
    b.viaNum() = 5;
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(a > b);
}

TEST(QueueCostTest, TieBreakByDistance) {
    QueueCost a;
    a.congestion() = 1;
    a.viaNum() = 2;
    a.distance() = 10;
    QueueCost b;
    b.congestion() = 1;
    b.viaNum() = 2;
    b.distance() = 20;
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(a > b);
}

TEST(QueueCostTest, TieBreakByRandomization) {
    QueueCost a;
    a.congestion() = 1;
    a.viaNum() = 2;
    a.distance() = 10;
    a.randomization() = 3;
    QueueCost b;
    b.congestion() = 1;
    b.viaNum() = 2;
    b.distance() = 10;
    b.randomization() = 7;
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(a > b);
}

TEST(QueueCostTest, EqualCostsNeitherLessNorGreater) {
    QueueCost a;
    a.congestion() = 1;
    a.viaNum() = 2;
    a.distance() = 10;
    QueueCost b;
    b.congestion() = 1;
    b.viaNum() = 2;
    b.distance() = 10;
    EXPECT_FALSE(a < b);
    EXPECT_FALSE(a > b);
}

TEST(QueueCostTest, PrintOutput) {
    QueueCost c;
    c.congestion() = 5;
    c.viaNum() = 3;
    c.distance() = 100;
    c.randomization() = 7;
    std::ostringstream oss;
    c.print(oss);
    std::string const output = oss.str();
    EXPECT_NE(output.find('5'), std::string::npos);
    EXPECT_NE(output.find('3'), std::string::npos);
    EXPECT_NE(output.find("100"), std::string::npos);
    EXPECT_NE(output.find('7'), std::string::npos);
}

TEST(QueueCostTest, MemorySetsIndependent) {
    QueueCost c;
    c.memorySource().insert(1);
    c.memorySource().insert(2);
    c.futureMemorySource().insert(3);
    c.memoryTarget().insert(4);
    c.futureMemoryTarget().insert(5);

    EXPECT_EQ(c.memorySource().size(), 2U);
    EXPECT_EQ(c.futureMemorySource().size(), 1U);
    EXPECT_EQ(c.memoryTarget().size(), 1U);
    EXPECT_EQ(c.futureMemoryTarget().size(), 1U);
}
