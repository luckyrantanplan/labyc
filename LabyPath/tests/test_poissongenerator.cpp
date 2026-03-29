/**
 * @file test_poissongenerator.cpp
 * @brief Unit tests for PoissonGenerator point structures and grid.
 */

#include "generator/PoissonGenerator.h"
#include <cmath>
#include <cstddef>
#include <gtest/gtest.h>

using laby::generator::PoissonPoints;
using laby::generator::SGrid;
using laby::generator::SGridPoint;
using laby::generator::SPoint;

// ── sPoint ─────────────────────────────────────────────────────────────────

TEST(SPointTest, DefaultConstruction) {
    SPoint const p;
    EXPECT_DOUBLE_EQ(p.xCoord(), 0.0);
    EXPECT_DOUBLE_EQ(p.yCoord(), 0.0);
    EXPECT_FALSE(p.isValid());
}

TEST(SPointTest, ParameterizedConstruction) {
    SPoint const p(SPoint::Coordinates{0.5, 0.3});
    EXPECT_DOUBLE_EQ(p.xCoord(), 0.5);
    EXPECT_DOUBLE_EQ(p.yCoord(), 0.3);
    EXPECT_TRUE(p.isValid());
}

TEST(SPointTest, IsInRectangleInside) {
    EXPECT_TRUE(SPoint(SPoint::Coordinates{0.5, 0.5}).isInRectangle());
    EXPECT_TRUE(SPoint(SPoint::Coordinates{0.0, 0.0}).isInRectangle());
    EXPECT_TRUE(SPoint(SPoint::Coordinates{1.0, 1.0}).isInRectangle());
}

TEST(SPointTest, IsInRectangleOutside) {
    EXPECT_FALSE(SPoint(SPoint::Coordinates{-0.1, 0.5}).isInRectangle());
    EXPECT_FALSE(SPoint(SPoint::Coordinates{0.5, 1.1}).isInRectangle());
    EXPECT_FALSE(SPoint(SPoint::Coordinates{1.1, 0.5}).isInRectangle());
}

TEST(SPointTest, IsInCircleCenter) {
    EXPECT_TRUE(SPoint(SPoint::Coordinates{0.5, 0.5}).isInCircle());
}

TEST(SPointTest, IsInCircleBoundary) {
    // On the boundary: distance from center (0.5, 0.5) to (1.0, 0.5) is 0.5
    EXPECT_TRUE(SPoint(SPoint::Coordinates{1.0, 0.5}).isInCircle());
}

TEST(SPointTest, IsInCircleOutside) {
    EXPECT_FALSE(SPoint(SPoint::Coordinates{0.0, 0.0}).isInCircle());
    EXPECT_FALSE(SPoint(SPoint::Coordinates{1.0, 1.0}).isInCircle());
}

// ── SGrid ──────────────────────────────────────────────────────────────────

TEST(SGridTest, ImageToGrid) {
    SGrid const grid(SGrid::Dimensions{10, 10}, 0.1);
    SPoint const p(SPoint::Coordinates{0.25, 0.35});
    SGridPoint const gp = laby::generator::SGrid::imageToGrid(p, 0.1);
    EXPECT_EQ(gp.xIndex(), 2);
    EXPECT_EQ(gp.yIndex(), 3);
}

TEST(SGridTest, GetSqDistance) {
    SGrid const grid(SGrid::Dimensions{10, 10}, 0.1);
    SPoint const a(SPoint::Coordinates{0.0, 0.0});
    SPoint const b(SPoint::Coordinates{3.0, 4.0});
    EXPECT_DOUBLE_EQ(grid.getSqDistance(a, b), 25.0);
}

TEST(SGridTest, GetSqDistanceSamePoint) {
    SGrid const grid(SGrid::Dimensions{10, 10}, 0.1);
    SPoint const a(SPoint::Coordinates{1.0, 1.0});
    EXPECT_DOUBLE_EQ(grid.getSqDistance(a, a), 0.0);
}

TEST(SGridTest, InsertAndNeighbourhood) {
    double const cellSize = 0.1;
    int const width = static_cast<int>(std::ceil(1.0 / cellSize));
    int const height = width;
    SGrid grid(SGrid::Dimensions{width, height}, cellSize);

    SPoint const p(SPoint::Coordinates{0.5, 0.5});
    grid.insert(p);

    // A nearby point should be in neighbourhood
    SPoint const nearby(SPoint::Coordinates{0.51, 0.51});
    EXPECT_TRUE(grid.isInNeighbourhood(nearby, 0.01));

    // A far point should not be in neighbourhood
    SPoint const far(SPoint::Coordinates{0.1, 0.1});
    EXPECT_FALSE(grid.isInNeighbourhood(far, 0.001));
}

// ── PoissonPoints generation ──────────────────────────────────────────────

TEST(PoissonPointsTest, GenerateProducesPoints) {
    auto points = PoissonPoints::generate(50, PoissonPoints::GenerateConfig{.circle = false});
    EXPECT_GT(points.size(), 0U);
    EXPECT_LE(points.size(), 50U);
}

TEST(PoissonPointsTest, GeneratedPointsInRectangle) {
    auto points = PoissonPoints::generate(100, PoissonPoints::GenerateConfig{.circle = false});
    for (const auto& p : points) {
        EXPECT_GE(p.xCoord(), 0.0);
        EXPECT_LE(p.xCoord(), 1.0);
        EXPECT_GE(p.yCoord(), 0.0);
        EXPECT_LE(p.yCoord(), 1.0);
    }
}

TEST(PoissonPointsTest, GeneratedPointsMinDistance) {
    auto points = PoissonPoints::generate(30, PoissonPoints::GenerateConfig{.circle = false});
    double const minDist = std::sqrt(6.3 / 300.0);    // approximate MinDist for 30 points
    double const sqMinDist = minDist * minDist * 0.9; // slight tolerance

    for (size_t i = 0; i < points.size(); ++i) {
        for (size_t j = i + 1; j < points.size(); ++j) {
            double const dx = points[i].xCoord() - points[j].xCoord();
            double const dy = points[i].yCoord() - points[j].yCoord();
            double const sqDist = dx * dx + dy * dy;
            EXPECT_GE(sqDist, sqMinDist * 0.5);
        }
    }
}
