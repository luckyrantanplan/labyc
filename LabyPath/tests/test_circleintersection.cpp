/**
 * @file test_circleintersection.cpp
 * @brief Unit tests for CircleIntersection geometry operations
 */

#include <gtest/gtest.h>
#include "basic/CircleIntersection.h"

namespace laby {
namespace basic {
namespace {

using K = CircleIntersection::Kernel;

TEST(CircleIntersectionTest, TwoIntersections) {
    // Circle at origin with radius 5, line segment from (-10,0) to (10,0)
    K::Point_2 center(0, 0);
    K::Point_2 a(-10, 0);
    K::Point_2 b(10, 0);

    auto result = CircleIntersection::prob2(center, 5.0, a, b);
    ASSERT_EQ(result.size(), 2U);

    // Intersection points should be at (-5, 0) and (5, 0)
    // The result should be ordered by distance to 'a' (closest first)
    double x0 = CGAL::to_double(result[0].x());
    double y0 = CGAL::to_double(result[0].y());
    double x1 = CGAL::to_double(result[1].x());
    double y1 = CGAL::to_double(result[1].y());

    EXPECT_NEAR(y0, 0.0, 1e-6);
    EXPECT_NEAR(y1, 0.0, 1e-6);
    // Closest to a=(-10,0) should be first
    EXPECT_LT(x0, x1);
}

TEST(CircleIntersectionTest, NoIntersection) {
    // Circle at origin with radius 1, line far away
    K::Point_2 center(0, 0);
    K::Point_2 a(10, 10);
    K::Point_2 b(20, 10);

    auto result = CircleIntersection::prob2(center, 1.0, a, b);
    EXPECT_EQ(result.size(), 0U);
}

TEST(CircleIntersectionTest, TangentIntersection) {
    // Circle at origin with radius 5, horizontal line at y=5
    K::Point_2 center(0, 0);
    K::Point_2 a(-10, 5);
    K::Point_2 b(10, 5);

    auto result = CircleIntersection::prob2(center, 5.0, a, b);
    // Tangent should produce 1 intersection point
    ASSERT_EQ(result.size(), 1U);
    double x0 = CGAL::to_double(result[0].x());
    double y0 = CGAL::to_double(result[0].y());
    EXPECT_NEAR(x0, 0.0, 1e-6);
    EXPECT_NEAR(y0, 5.0, 1e-6);
}

TEST(CircleIntersectionTest, VerticalLine) {
    // Circle at (5, 5) with radius 3, vertical line x=5
    K::Point_2 center(5, 5);
    K::Point_2 a(5, -10);
    K::Point_2 b(5, 20);

    auto result = CircleIntersection::prob2(center, 3.0, a, b);
    ASSERT_EQ(result.size(), 2U);

    double y0 = CGAL::to_double(result[0].y());
    double y1 = CGAL::to_double(result[1].y());
    // Points should be at (5, 2) and (5, 8)
    EXPECT_NEAR(std::min(y0, y1), 2.0, 1e-6);
    EXPECT_NEAR(std::max(y0, y1), 8.0, 1e-6);
}

} // namespace
} // namespace basic
} // namespace laby
