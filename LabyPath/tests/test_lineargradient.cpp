/**
 * @file test_lineargradient.cpp
 * @brief Unit tests for LinearGradient thickness interpolation
 */

#include <gtest/gtest.h>
#include "GeomData.h"
#include "basic/LinearGradient.h"


namespace laby::basic {
namespace {

TEST(LinearGradientTest, UniformThickness) {
    Point_2 const p1(0, 0);
    Point_2 const p2(10, 0);
    LinearGradient grad(p1, 5.0, p2, 5.0);

    // At any point along the segment, thickness should be 5.0
    EXPECT_NEAR(grad.thickness(Point_2(0, 0)), 5.0, 1e-6);
    EXPECT_NEAR(grad.thickness(Point_2(5, 0)), 5.0, 1e-6);
    EXPECT_NEAR(grad.thickness(Point_2(10, 0)), 5.0, 1e-6);
}

TEST(LinearGradientTest, AtEndpoints) {
    Point_2 const p1(0, 0);
    Point_2 const p2(10, 0);
    LinearGradient grad(p1, 2.0, p2, 8.0);

    // At origin, thickness should be thickness1
    EXPECT_NEAR(grad.thickness(Point_2(0, 0)), 2.0, 1e-6);
    // At target, thickness should be thickness2
    EXPECT_NEAR(grad.thickness(Point_2(10, 0)), 8.0, 1e-6);
}

TEST(LinearGradientTest, BeforeOrigin) {
    Point_2 const p1(0, 0);
    Point_2 const p2(10, 0);
    LinearGradient grad(p1, 2.0, p2, 8.0);

    // Before origin (t <= 0) should return thickness1
    EXPECT_NEAR(grad.thickness(Point_2(-5, 0)), 2.0, 1e-6);
}

TEST(LinearGradientTest, BeyondTarget) {
    Point_2 const p1(0, 0);
    Point_2 const p2(10, 0);
    LinearGradient grad(p1, 2.0, p2, 8.0);

    // Beyond target (t >= 1) should return thickness2
    EXPECT_NEAR(grad.thickness(Point_2(15, 0)), 8.0, 1e-6);
}

TEST(LinearGradientTest, InterpolationMonotonic) {
    Point_2 const p1(0, 0);
    Point_2 const p2(10, 0);
    LinearGradient grad(p1, 2.0, p2, 8.0);

    // Thickness should increase monotonically from p1 to p2
    double prev = grad.thickness(Point_2(0, 0));
    for (int i = 1; i <= 10; ++i) {
        double const curr = grad.thickness(Point_2(i, 0));
        EXPECT_GE(curr, prev);
        prev = curr;
    }
}

TEST(LinearGradientTest, OriginStoredByValue) {
    // This test verifies that the origin point is stored by value,
    // not by reference (which would cause a dangling reference bug).
    LinearGradient* grad = nullptr;
    {
        Point_2 const p1(0, 0);
        Point_2 const p2(10, 0);
        grad = new LinearGradient(p1, 2.0, p2, 8.0);
        // p1 goes out of scope here
    }
    // If origin was stored by reference, this would access freed memory
    double const t = grad->thickness(Point_2(5, 0));
    EXPECT_GE(t, 2.0);
    EXPECT_LE(t, 8.0);
    delete grad;
}

TEST(LinearGradientTest, FFunction) {
    Point_2 const p1(0, 0);
    Point_2 const p2(10, 0);
    LinearGradient const grad(p1, 2.0, p2, 8.0);

    // f(0, a, b) should return a (t^4 = 0)
    EXPECT_NEAR(grad.f(0.0, 2.0, 8.0), 2.0, 1e-9);
    // f(1, a, b) should return b (t^4 = 1)
    EXPECT_NEAR(grad.f(1.0, 2.0, 8.0), 8.0, 1e-9);
}

TEST(LinearGradientTest, DiagonalSegment) {
    Point_2 const p1(0, 0);
    Point_2 const p2(10, 10);
    LinearGradient grad(p1, 1.0, p2, 9.0);

    EXPECT_NEAR(grad.thickness(Point_2(0, 0)), 1.0, 1e-6);
    EXPECT_NEAR(grad.thickness(Point_2(10, 10)), 9.0, 1e-6);
}

} // namespace
} // namespace laby::basic

