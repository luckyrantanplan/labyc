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
    EXPECT_DOUBLE_EQ(p.x, 0.0);
    EXPECT_DOUBLE_EQ(p.y, 0.0);
    EXPECT_FALSE(p.m_Valid);
}

TEST(SPointTest, ParameterizedConstruction) {
    SPoint const p(0.5, 0.3);
    EXPECT_DOUBLE_EQ(p.x, 0.5);
    EXPECT_DOUBLE_EQ(p.y, 0.3);
    EXPECT_TRUE(p.m_Valid);
}

TEST(SPointTest, IsInRectangleInside) {
    EXPECT_TRUE(SPoint(0.5, 0.5).isInRectangle());
    EXPECT_TRUE(SPoint(0.0, 0.0).isInRectangle());
    EXPECT_TRUE(SPoint(1.0, 1.0).isInRectangle());
}

TEST(SPointTest, IsInRectangleOutside) {
    EXPECT_FALSE(SPoint(-0.1, 0.5).isInRectangle());
    EXPECT_FALSE(SPoint(0.5, 1.1).isInRectangle());
    EXPECT_FALSE(SPoint(1.1, 0.5).isInRectangle());
}

TEST(SPointTest, IsInCircleCenter) {
    EXPECT_TRUE(SPoint(0.5, 0.5).isInCircle());
}

TEST(SPointTest, IsInCircleBoundary) {
    // On the boundary: distance from center (0.5, 0.5) to (1.0, 0.5) is 0.5
    EXPECT_TRUE(SPoint(1.0, 0.5).isInCircle());
}

TEST(SPointTest, IsInCircleOutside) {
    EXPECT_FALSE(SPoint(0.0, 0.0).isInCircle());
    EXPECT_FALSE(SPoint(1.0, 1.0).isInCircle());
}

// ── SGrid ──────────────────────────────────────────────────────────────────

TEST(SGridTest, ImageToGrid) {
    SGrid const grid(10, 10, 0.1);
    SPoint const p(0.25, 0.35);
    SGridPoint const gp = laby::generator::SGrid::imageToGrid(p, 0.1);
    EXPECT_EQ(gp.x, 2);
    EXPECT_EQ(gp.y, 3);
}

TEST(SGridTest, GetSqDistance) {
    SGrid const grid(10, 10, 0.1);
    SPoint const a(0.0, 0.0);
    SPoint const b(3.0, 4.0);
    EXPECT_DOUBLE_EQ(grid.getSqDistance(a, b), 25.0);
}

TEST(SGridTest, GetSqDistanceSamePoint) {
    SGrid const grid(10, 10, 0.1);
    SPoint const a(1.0, 1.0);
    EXPECT_DOUBLE_EQ(grid.getSqDistance(a, a), 0.0);
}

TEST(SGridTest, InsertAndNeighbourhood) {
    double const cellSize = 0.1;
    int const w = static_cast<int>(std::ceil(1.0 / cellSize));
    int const h = w;
    SGrid grid(w, h, cellSize);

    SPoint const p(0.5, 0.5);
    grid.insert(p);

    // A nearby point should be in neighbourhood
    SPoint const nearby(0.51, 0.51);
    EXPECT_TRUE(grid.isInNeighbourhood(nearby, 0.01, cellSize));

    // A far point should not be in neighbourhood
    SPoint const far(0.1, 0.1);
    EXPECT_FALSE(grid.isInNeighbourhood(far, 0.001, cellSize));
}

// ── PoissonPoints generation ──────────────────────────────────────────────

TEST(PoissonPointsTest, GenerateProducesPoints) {
    auto points = PoissonPoints::generate(50, 30, false);
    EXPECT_GT(points.size(), 0U);
    EXPECT_LE(points.size(), 50U);
}

TEST(PoissonPointsTest, GeneratedPointsInRectangle) {
    auto points = PoissonPoints::generate(100, 30, false);
    for (const auto& p : points) {
        EXPECT_GE(p.x, 0.0);
        EXPECT_LE(p.x, 1.0);
        EXPECT_GE(p.y, 0.0);
        EXPECT_LE(p.y, 1.0);
    }
}

TEST(PoissonPointsTest, GeneratedPointsMinDistance) {
    auto points = PoissonPoints::generate(30, 30, false);
    double const minDist = std::sqrt(6.3 / 300.0);    // approximate MinDist for 30 points
    double const sqMinDist = minDist * minDist * 0.9; // slight tolerance

    for (size_t i = 0; i < points.size(); ++i) {
        for (size_t j = i + 1; j < points.size(); ++j) {
            double const dx = points[i].x - points[j].x;
            double const dy = points[i].y - points[j].y;
            double const sqDist = dx * dx + dy * dy;
            EXPECT_GE(sqDist, sqMinDist * 0.5);
        }
    }
}
