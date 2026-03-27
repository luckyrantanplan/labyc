/**
 * @file test_numericrange.cpp
 * @brief Unit tests for NumericRange iteration and NumericHelper
 */

#include "basic/NumericRange.h"
#include <gtest/gtest.h>
#include <vector>

namespace laby {
namespace {

TEST(NumericRangeTest, IntegerRange) {
    NumericRange<int32_t> range(0, 10, 2);
    std::vector<int32_t> values;
    for (int32_t v : range) {
        values.push_back(v);
    }
    // 0, 2, 4, 6, 8, 10
    ASSERT_EQ(values.size(), 6U);
    EXPECT_EQ(values[0], 0);
    EXPECT_EQ(values[1], 2);
    EXPECT_EQ(values[2], 4);
    EXPECT_EQ(values[3], 6);
    EXPECT_EQ(values[4], 8);
    EXPECT_EQ(values[5], 10);
}

TEST(NumericRangeTest, StepOne) {
    NumericRange<int32_t> range(0, 3, 1);
    std::vector<int32_t> values;
    for (int32_t v : range) {
        values.push_back(v);
    }
    // 0, 1, 2, 3
    ASSERT_EQ(values.size(), 4U);
    EXPECT_EQ(values[0], 0);
    EXPECT_EQ(values[3], 3);
}

TEST(NumericRangeTest, DoubleRange) {
    NumericRange<double> range(0.0, 1.0, 0.25);
    std::vector<double> values;
    for (double v : range) {
        values.push_back(v);
    }
    // 0.0, 0.25, 0.5, 0.75, 1.0
    ASSERT_EQ(values.size(), 5U);
    EXPECT_NEAR(values[0], 0.0, 1e-9);
    EXPECT_NEAR(values[1], 0.25, 1e-9);
    EXPECT_NEAR(values[4], 1.0, 1e-9);
}

TEST(NumericRangeTest, GetValue) {
    NumericRange<int32_t> range(10, 20, 5);
    EXPECT_EQ(range.getValue(0), 10);
    EXPECT_EQ(range.getValue(1), 15);
    EXPECT_EQ(range.getValue(2), 20);
}

TEST(NumericHelperTest, ReduceAtZero) {
    auto result = NumericHelper::reduce(0, 4, 8);
    EXPECT_EQ(result, std::optional<int32_t>{0});
}

TEST(NumericHelperTest, ReduceProducesValue) {
    // x=1, detailSize=2, globalSize=4
    // previous = (0*4)/2 = 0, current = (1*4)/2 = 2
    // previous != current, so return current
    auto result = NumericHelper::reduce(1, 2, 4);
    EXPECT_EQ(result, std::optional<int32_t>{2});
}

TEST(NumericHelperTest, ReduceSkipsValue) {
    // x=2, detailSize=4, globalSize=2
    // previous = (1*2)/4 = 0, current = (2*2)/4 = 1
    // previous != current, so return 1
    auto r1 = NumericHelper::reduce(2, 4, 2);
    ASSERT_TRUE(r1.has_value());

    // x=3, detailSize=4, globalSize=2
    // previous = (2*2)/4 = 1, current = (3*2)/4 = 1
    // previous == current, so return nullopt
    auto r2 = NumericHelper::reduce(3, 4, 2);
    EXPECT_FALSE(r2.has_value());
}

} // namespace
} // namespace laby
