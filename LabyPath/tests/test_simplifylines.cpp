/**
 * @file test_simplifylines.cpp
 * @brief Unit tests for SimplifyLines (Boost.Geometry line decimation).
 */

#include "basic/SimplifyLines.h"
#include <cmath>
#include <cstddef>
#include <gtest/gtest.h>

using laby::SimplifyLines;

namespace {

constexpr int kDensePointCount = 100;
constexpr int kIndexedPointCount = 50;
constexpr double kDenseSpacing = 0.1;
constexpr double kDecimationDistance = 1.0;
constexpr double kEndpointPreservingDistance = 0.5;
constexpr double kSmallDistance = 0.001;
constexpr double kIntermediateX = 1.0;
constexpr double kIntermediateY = 0.5;
constexpr double kSecondIntermediateX = 2.0;
constexpr double kSecondIntermediateY = 0.1;
constexpr double kThirdIntermediateX = 3.0;
constexpr double kFinalX = 10.0;
constexpr double kDiagonalX = 5.0;
constexpr double kDiagonalY = 5.0;
constexpr std::size_t kDenseLineSize = 101U;
constexpr std::size_t kIndexedLineSize = 51U;
constexpr std::size_t kMinimumSimplifiedSize = 2U;
constexpr std::size_t kThreePointSize = 3U;

} // namespace

TEST(SimplifyLinesTest, DecimateReducesPoints) {
    SimplifyLines::LineString line;
    // Create a line with many closely-spaced points on x-axis
    for (int pointIndex = 0; pointIndex <= kDensePointCount; ++pointIndex) {
        double const xCoordinate = static_cast<double>(pointIndex) * kDenseSpacing;
        line.push_back(SimplifyLines::xy(xCoordinate, 0.0));
    }
    ASSERT_EQ(line.size(), kDenseLineSize);

    auto const simplified = SimplifyLines::decimate(line, kDecimationDistance);
    EXPECT_LT(simplified.size(), kDenseLineSize);
    EXPECT_GT(simplified.size(), 0U);
}

TEST(SimplifyLinesTest, DecimatePreservesEndpoints) {
    SimplifyLines::LineString line;
    line.push_back(SimplifyLines::xy(0.0, 0.0));
    line.push_back(SimplifyLines::xy(kIntermediateX, kIntermediateY));
    line.push_back(SimplifyLines::xy(kSecondIntermediateX, kSecondIntermediateY));
    line.push_back(SimplifyLines::xy(kThirdIntermediateX, 0.0));
    line.push_back(SimplifyLines::xy(kFinalX, 0.0));

    auto const simplified = SimplifyLines::decimate(line, kEndpointPreservingDistance);
    EXPECT_GE(simplified.size(), kMinimumSimplifiedSize);

    // First and last points should be preserved
    EXPECT_DOUBLE_EQ(simplified.front().x(), 0.0);
    EXPECT_DOUBLE_EQ(simplified.back().x(), kFinalX);
}

TEST(SimplifyLinesTest, DecimateSmallDistanceKeepsAll) {
    SimplifyLines::LineString line;
    line.push_back(SimplifyLines::xy(0.0, 0.0));
    line.push_back(SimplifyLines::xy(kDiagonalX, kDiagonalY));
    line.push_back(SimplifyLines::xy(kFinalX, 0.0));

    auto const simplified = SimplifyLines::decimate(line, kSmallDistance);
    EXPECT_EQ(simplified.size(), kThreePointSize);
}

TEST(SimplifyLinesTest, DecimateIndexReducesPoints) {
    SimplifyLines::LineStringIndexed line;
    for (int pointIndex = 0; pointIndex <= kIndexedPointCount; ++pointIndex) {
        double const xCoordinate = static_cast<double>(pointIndex) * kDenseSpacing;
        line.push_back(laby::IndexedPoint::fromCoordinates({xCoordinate, 0.0},
                                                           static_cast<std::size_t>(pointIndex)));
    }
    ASSERT_EQ(line.size(), kIndexedLineSize);

    auto const simplified = SimplifyLines::decimateIndex(line, kEndpointPreservingDistance);
    EXPECT_LT(simplified.size(), kIndexedLineSize);
    EXPECT_GT(simplified.size(), 0U);
}

TEST(SimplifyLinesTest, DecimateIndexPreservesIndices) {
    SimplifyLines::LineStringIndexed line;
    line.push_back(laby::IndexedPoint::fromCoordinates({0.0, 0.0}, 0));
    line.push_back(laby::IndexedPoint::fromCoordinates({kDiagonalX, kDiagonalY}, 1));
    line.push_back(laby::IndexedPoint::fromCoordinates({kFinalX, 0.0}, 2));

    auto const simplified = SimplifyLines::decimateIndex(line, kSmallDistance);
    EXPECT_EQ(simplified.size(), kThreePointSize);
    EXPECT_EQ(simplified.front().index(), 0U);
    EXPECT_EQ(simplified.back().index(), 2U);
}
