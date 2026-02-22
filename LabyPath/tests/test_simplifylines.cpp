/**
 * @file test_simplifylines.cpp
 * @brief Unit tests for SimplifyLines (Boost.Geometry line decimation).
 */

#include <gtest/gtest.h>
#include <cmath>
#include <boost/geometry.hpp>
#include "basic/SimplifyLines.h"

using laby::SimplifyLines;

TEST(SimplifyLinesTest, DecimateReducesPoints) {
    SimplifyLines::LineString line;
    // Create a line with many closely-spaced points on x-axis
    for (int i = 0; i <= 100; ++i) {
        double x = static_cast<double>(i) * 0.1;
        line.push_back(SimplifyLines::xy(x, 0.0));
    }
    ASSERT_EQ(line.size(), 101u);

    auto simplified = SimplifyLines::decimate(line, 1.0);
    EXPECT_LT(simplified.size(), 101u);
    EXPECT_GT(simplified.size(), 0u);
}

TEST(SimplifyLinesTest, DecimatePreservesEndpoints) {
    SimplifyLines::LineString line;
    line.push_back(SimplifyLines::xy(0.0, 0.0));
    line.push_back(SimplifyLines::xy(1.0, 0.5));
    line.push_back(SimplifyLines::xy(2.0, 0.1));
    line.push_back(SimplifyLines::xy(3.0, 0.0));
    line.push_back(SimplifyLines::xy(10.0, 0.0));

    auto simplified = SimplifyLines::decimate(line, 0.5);
    EXPECT_GE(simplified.size(), 2u);

    // First and last points should be preserved
    EXPECT_DOUBLE_EQ(simplified.front().x(), 0.0);
    EXPECT_DOUBLE_EQ(simplified.back().x(), 10.0);
}

TEST(SimplifyLinesTest, DecimateSmallDistanceKeepsAll) {
    SimplifyLines::LineString line;
    line.push_back(SimplifyLines::xy(0.0, 0.0));
    line.push_back(SimplifyLines::xy(5.0, 5.0));
    line.push_back(SimplifyLines::xy(10.0, 0.0));

    auto simplified = SimplifyLines::decimate(line, 0.001);
    EXPECT_EQ(simplified.size(), 3u);
}

TEST(SimplifyLinesTest, DecimateIndexReducesPoints) {
    SimplifyLines::LineStringIndexed line;
    for (int i = 0; i <= 50; ++i) {
        double x = static_cast<double>(i) * 0.1;
        line.push_back(laby::Indexed_Point(x, 0.0, static_cast<std::size_t>(i)));
    }
    ASSERT_EQ(line.size(), 51u);

    auto simplified = SimplifyLines::decimateIndex(line, 0.5);
    EXPECT_LT(simplified.size(), 51u);
    EXPECT_GT(simplified.size(), 0u);
}

TEST(SimplifyLinesTest, DecimateIndexPreservesIndices) {
    SimplifyLines::LineStringIndexed line;
    line.push_back(laby::Indexed_Point(0.0, 0.0, 0));
    line.push_back(laby::Indexed_Point(5.0, 5.0, 1));
    line.push_back(laby::Indexed_Point(10.0, 0.0, 2));

    auto simplified = SimplifyLines::decimateIndex(line, 0.001);
    EXPECT_EQ(simplified.size(), 3u);
    EXPECT_EQ(simplified.front().index, 0u);
    EXPECT_EQ(simplified.back().index, 2u);
}
