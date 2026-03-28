/**
 * @file test_smoothing.cpp
 * @brief Unit tests for Smoothing class (Chaikin curve smoothing)
 */

#include <gtest/gtest.h>
#include "Polyline.h"
#include "Smoothing.h"

namespace laby {
namespace {


constexpr double kDefaultTension = 0.5;
constexpr double kDirectSmootherTension = 0.25;
constexpr double kNegativeTension = -1.0;
constexpr int kMinimumInputPoints = 3;
constexpr int kSingleIteration = 1;
constexpr int kDoubleIterations = 2;
constexpr int kZeroIterations = 0;
constexpr std::size_t kTwoPoints = 2U;
constexpr int kMidpointCoordinate = 5;
constexpr int kFarCoordinate = 10;

TEST(SmoothingTest, ShortPolylineUnchanged) {
    // A polyline with fewer than 3 points should be returned unchanged
    Polyline polyline;
    polyline.points().emplace_back(0, 0);
    polyline.points().emplace_back(1, 1);

    Polyline const result =
        Smoothing::getCurveSmoothingChaikin(polyline, kDefaultTension, kMinimumInputPoints);
    EXPECT_EQ(result.points().size(), kTwoPoints);
    EXPECT_EQ(result.points()[0], Point_2(0, 0));
    EXPECT_EQ(result.points()[1], Point_2(1, 1));
}

TEST(SmoothingTest, EmptyPolylineUnchanged) {
    Polyline const polyline;
    Polyline const result =
        Smoothing::getCurveSmoothingChaikin(polyline, kDefaultTension, kMinimumInputPoints);
    EXPECT_TRUE(result.points().empty());
}

TEST(SmoothingTest, SingleIterationIncreasesPoints) {
    Polyline polyline;
    polyline.points().emplace_back(0, 0);
    polyline.points().emplace_back(kMidpointCoordinate, 0);
    polyline.points().emplace_back(kMidpointCoordinate, kMidpointCoordinate);

    Polyline const result =
        Smoothing::getCurveSmoothingChaikin(polyline, kDefaultTension, kSingleIteration);
    // Chaikin smoothing should produce more points than original
    EXPECT_GT(result.points().size(), polyline.points().size());
}

TEST(SmoothingTest, MoreIterationsMorePoints) {
    Polyline polyline;
    polyline.points().emplace_back(0, 0);
    polyline.points().emplace_back(kMidpointCoordinate, 0);
    polyline.points().emplace_back(kMidpointCoordinate, kMidpointCoordinate);
    polyline.points().emplace_back(0, kMidpointCoordinate);

    Polyline const singleIterationResult =
        Smoothing::getCurveSmoothingChaikin(polyline, kDefaultTension, kSingleIteration);
    Polyline const doubleIterationResult =
        Smoothing::getCurveSmoothingChaikin(polyline, kDefaultTension, kDoubleIterations);
    EXPECT_GT(doubleIterationResult.points().size(), singleIterationResult.points().size());
}

TEST(SmoothingTest, EndpointsPreserved) {
    // For an open polyline, first and last points should be preserved
    Polyline polyline;
    polyline.points().emplace_back(0, 0);
    polyline.points().emplace_back(kMidpointCoordinate, 0);
    polyline.points().emplace_back(kFarCoordinate, kMidpointCoordinate);

    Polyline const result =
        Smoothing::getCurveSmoothingChaikin(polyline, kDefaultTension, kSingleIteration);
    EXPECT_EQ(result.points().front(), Point_2(0, 0));
    EXPECT_EQ(result.points().back(), Point_2(kFarCoordinate, kMidpointCoordinate));
}

TEST(SmoothingTest, NegativeTensionClampedToZero) {
    Polyline polyline;
    polyline.points().emplace_back(0, 0);
    polyline.points().emplace_back(kMidpointCoordinate, 0);
    polyline.points().emplace_back(kMidpointCoordinate, kMidpointCoordinate);

    // Negative tension should be clamped to 0 and still produce valid output
    Polyline const result =
        Smoothing::getCurveSmoothingChaikin(polyline, kNegativeTension, kSingleIteration);
    EXPECT_GT(result.points().size(), kTwoPoints);
    EXPECT_EQ(result.points().front(), Point_2(0, 0));
    EXPECT_EQ(result.points().back(), Point_2(kMidpointCoordinate, kMidpointCoordinate));
}

TEST(SmoothingTest, ZeroIterationsUnchanged) {
    Polyline polyline;
    polyline.points().emplace_back(0, 0);
    polyline.points().emplace_back(kMidpointCoordinate, 0);
    polyline.points().emplace_back(kMidpointCoordinate, kMidpointCoordinate);

    Polyline const result =
        Smoothing::getCurveSmoothingChaikin(polyline, kDefaultTension, kZeroIterations);
    EXPECT_EQ(result.points().size(), polyline.points().size());
}

TEST(SmoothingTest, ClosedPolylineSmoothing) {
    Polyline polyline;
    polyline.setClosed(true);
    polyline.points().emplace_back(0, 0);
    polyline.points().emplace_back(kFarCoordinate, 0);
    polyline.points().emplace_back(kFarCoordinate, kFarCoordinate);
    polyline.points().emplace_back(0, 0); // close the loop

    Polyline const result =
        Smoothing::getCurveSmoothingChaikin(polyline, kDefaultTension, kSingleIteration);
    // For a closed polyline, first and last point should be the same
    EXPECT_EQ(result.points().front(), result.points().back());
}

TEST(SmoothingTest, GetSmootherChaikinDirect) {
    Polyline polyline;
    polyline.points().emplace_back(0, 0);
    polyline.points().emplace_back(kMidpointCoordinate, 0);
    polyline.points().emplace_back(kMidpointCoordinate, kMidpointCoordinate);

    Polyline const result = Smoothing::getSmootherChaikin(polyline, kDirectSmootherTension);
    // Should produce more points
    EXPECT_GT(result.points().size(), polyline.points().size());
    // Endpoints preserved for open polyline
    EXPECT_EQ(result.points().front(), Point_2(0, 0));
    EXPECT_EQ(result.points().back(), Point_2(kMidpointCoordinate, kMidpointCoordinate));
}

} // namespace
} // namespace laby
