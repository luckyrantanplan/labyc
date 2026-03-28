/**
 * @file test_smoothing.cpp
 * @brief Unit tests for Smoothing class (Chaikin curve smoothing)
 */

#include <gtest/gtest.h>
#include "Polyline.h"
#include "Smoothing.h"

namespace laby {
namespace {

TEST(SmoothingTest, ShortPolylineUnchanged) {
    // A polyline with fewer than 3 points should be returned unchanged
    Polyline pl;
    pl.points().emplace_back(0, 0);
    pl.points().emplace_back(1, 1);

    Polyline result = Smoothing::getCurveSmoothingChaikin(pl, 0.5, 3);
    EXPECT_EQ(result.points().size(), 2U);
    EXPECT_EQ(result.points()[0], Point_2(0, 0));
    EXPECT_EQ(result.points()[1], Point_2(1, 1));
}

TEST(SmoothingTest, EmptyPolylineUnchanged) {
    Polyline const pl;
    Polyline const result = Smoothing::getCurveSmoothingChaikin(pl, 0.5, 3);
    EXPECT_TRUE(result.points().empty());
}

TEST(SmoothingTest, SingleIterationIncreasesPoints) {
    Polyline pl;
    pl.points().emplace_back(0, 0);
    pl.points().emplace_back(5, 0);
    pl.points().emplace_back(5, 5);

    Polyline const result = Smoothing::getCurveSmoothingChaikin(pl, 0.5, 1);
    // Chaikin smoothing should produce more points than original
    EXPECT_GT(result.points().size(), pl.points().size());
}

TEST(SmoothingTest, MoreIterationsMorePoints) {
    Polyline pl;
    pl.points().emplace_back(0, 0);
    pl.points().emplace_back(5, 0);
    pl.points().emplace_back(5, 5);
    pl.points().emplace_back(0, 5);

    Polyline const r1 = Smoothing::getCurveSmoothingChaikin(pl, 0.5, 1);
    Polyline const r2 = Smoothing::getCurveSmoothingChaikin(pl, 0.5, 2);
    EXPECT_GT(r2.points().size(), r1.points().size());
}

TEST(SmoothingTest, EndpointsPreserved) {
    // For an open polyline, first and last points should be preserved
    Polyline pl;
    pl.points().emplace_back(0, 0);
    pl.points().emplace_back(5, 0);
    pl.points().emplace_back(10, 5);

    Polyline result = Smoothing::getCurveSmoothingChaikin(pl, 0.5, 1);
    EXPECT_EQ(result.points().front(), Point_2(0, 0));
    EXPECT_EQ(result.points().back(), Point_2(10, 5));
}

TEST(SmoothingTest, NegativeTensionClampedToZero) {
    Polyline pl;
    pl.points().emplace_back(0, 0);
    pl.points().emplace_back(5, 0);
    pl.points().emplace_back(5, 5);

    // Negative tension should be clamped to 0 and still produce valid output
    Polyline result = Smoothing::getCurveSmoothingChaikin(pl, -1.0, 1);
    EXPECT_GT(result.points().size(), 2U);
    EXPECT_EQ(result.points().front(), Point_2(0, 0));
    EXPECT_EQ(result.points().back(), Point_2(5, 5));
}

TEST(SmoothingTest, ZeroIterationsUnchanged) {
    Polyline pl;
    pl.points().emplace_back(0, 0);
    pl.points().emplace_back(5, 0);
    pl.points().emplace_back(5, 5);

    Polyline const result = Smoothing::getCurveSmoothingChaikin(pl, 0.5, 0);
    EXPECT_EQ(result.points().size(), pl.points().size());
}

TEST(SmoothingTest, ClosedPolylineSmoothing) {
    Polyline pl;
    pl.setClosed(true);
    pl.points().emplace_back(0, 0);
    pl.points().emplace_back(10, 0);
    pl.points().emplace_back(10, 10);
    pl.points().emplace_back(0, 0); // close the loop

    Polyline result = Smoothing::getCurveSmoothingChaikin(pl, 0.5, 1);
    // For a closed polyline, first and last point should be the same
    EXPECT_EQ(result.points().front(), result.points().back());
}

TEST(SmoothingTest, GetSmootherChaikinDirect) {
    Polyline pl;
    pl.points().emplace_back(0, 0);
    pl.points().emplace_back(5, 0);
    pl.points().emplace_back(5, 5);

    Polyline result = Smoothing::getSmootherChaikin(pl, 0.25);
    // Should produce more points
    EXPECT_GT(result.points().size(), pl.points().size());
    // Endpoints preserved for open polyline
    EXPECT_EQ(result.points().front(), Point_2(0, 0));
    EXPECT_EQ(result.points().back(), Point_2(5, 5));
}

} // namespace
} // namespace laby
