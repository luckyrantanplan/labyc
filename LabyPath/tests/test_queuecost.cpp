/**
 * @file test_queuecost.cpp
 * @brief Unit tests for QueueCost comparison and ordering.
 */

#include <gtest/gtest.h>
#include <sstream>
#include "Anisotrop/QueueCost.h"

using laby::aniso::QueueCost;

TEST(QueueCostTest, DefaultValues) {
    QueueCost c;
    EXPECT_EQ(c.distance, -1);
    EXPECT_EQ(c.via_num, 0);
    EXPECT_EQ(c.congestion, 0);
    EXPECT_EQ(c.randomization, 0);
}

TEST(QueueCostTest, LessThanByCongestion) {
    QueueCost a;
    a.congestion = 1;
    QueueCost b;
    b.congestion = 2;
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
}

TEST(QueueCostTest, GreaterThanByCongestion) {
    QueueCost a;
    a.congestion = 3;
    QueueCost b;
    b.congestion = 1;
    EXPECT_TRUE(a > b);
    EXPECT_FALSE(b > a);
}

TEST(QueueCostTest, TieBreakByVia) {
    QueueCost a;
    a.congestion = 1;
    a.via_num = 2;
    QueueCost b;
    b.congestion = 1;
    b.via_num = 5;
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(a > b);
}

TEST(QueueCostTest, TieBreakByDistance) {
    QueueCost a;
    a.congestion = 1;
    a.via_num = 2;
    a.distance = 10;
    QueueCost b;
    b.congestion = 1;
    b.via_num = 2;
    b.distance = 20;
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(a > b);
}

TEST(QueueCostTest, TieBreakByRandomization) {
    QueueCost a;
    a.congestion = 1;
    a.via_num = 2;
    a.distance = 10;
    a.randomization = 3;
    QueueCost b;
    b.congestion = 1;
    b.via_num = 2;
    b.distance = 10;
    b.randomization = 7;
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(a > b);
}

TEST(QueueCostTest, EqualCostsNeitherLessNorGreater) {
    QueueCost a;
    a.congestion = 1;
    a.via_num = 2;
    a.distance = 10;
    QueueCost b;
    b.congestion = 1;
    b.via_num = 2;
    b.distance = 10;
    EXPECT_FALSE(a < b);
    EXPECT_FALSE(a > b);
}

TEST(QueueCostTest, PrintOutput) {
    QueueCost c;
    c.congestion = 5;
    c.via_num = 3;
    c.distance = 100;
    c.randomization = 7;
    std::ostringstream oss;
    c.print(oss);
    std::string output = oss.str();
    EXPECT_NE(output.find("5"), std::string::npos);
    EXPECT_NE(output.find("3"), std::string::npos);
    EXPECT_NE(output.find("100"), std::string::npos);
    EXPECT_NE(output.find("7"), std::string::npos);
}

TEST(QueueCostTest, MemorySetsIndependent) {
    QueueCost c;
    c.memory_source.insert(1);
    c.memory_source.insert(2);
    c.future_memory_source.insert(3);
    c.memory_target.insert(4);
    c.future_memory_target.insert(5);

    EXPECT_EQ(c.memory_source.size(), 2u);
    EXPECT_EQ(c.future_memory_source.size(), 1u);
    EXPECT_EQ(c.memory_target.size(), 1u);
    EXPECT_EQ(c.future_memory_target.size(), 1u);
}
