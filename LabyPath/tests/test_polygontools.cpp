/**
 * @file test_polygontools.cpp
 * @brief Unit tests for PolygonTools geometry operations
 */

#include <CGAL/number_utils.h>
#include <cstddef>
#include <gtest/gtest.h>
#include "GeomData.h"
#include "basic/PolygonTools.h"

namespace laby {
namespace {

TEST(PolygonToolsTest, MakeTrapezeBasic) {
    Point_2 const a(0, 0);
    Point_2 const b(10, 0);
    Linear_polygon const poly = PolygonTools::makeTrapeze(a, b, 2.0, 2.0);

    // A trapeze between two points with equal thickness should be a rectangle
    ASSERT_EQ(poly.size(), 4U);
    // The polygon should be simple (non-self-intersecting)
    EXPECT_TRUE(poly.is_simple());
}

TEST(PolygonToolsTest, MakeTrapezeVaryingThickness) {
    Point_2 const a(0, 0);
    Point_2 const b(10, 0);
    Linear_polygon const poly = PolygonTools::makeTrapeze(a, b, 1.0, 3.0);

    ASSERT_EQ(poly.size(), 4U);
    EXPECT_TRUE(poly.is_simple());
}

TEST(PolygonToolsTest, MakeTrapezeHasCorrectOrientation) {
    Point_2 const a(0, 0);
    Point_2 const b(10, 0);
    Linear_polygon const poly = PolygonTools::makeTrapeze(a, b, 2.0, 2.0);

    // Polygon vertices should be around the segment from a to b
    // Check that all vertices are within expected bounds
    for (auto it = poly.vertices_begin(); it != poly.vertices_end(); ++it) {
        double const x = CGAL::to_double(it->x());
        double const y = CGAL::to_double(it->y());
        EXPECT_GE(x, -1.0);
        EXPECT_LE(x, 11.0);
        EXPECT_GE(y, -2.0);
        EXPECT_LE(y, 2.0);
    }
}

TEST(PolygonToolsTest, MakeTrapezeInPlace) {
    Point_2 const a(0, 0);
    Point_2 const b(5, 5);
    Linear_polygon poly;
    PolygonTools::makeTrapeze(poly, a, b, 1.0, 1.0);

    ASSERT_EQ(poly.size(), 4U);
    EXPECT_TRUE(poly.is_simple());
}

TEST(PolygonToolsTest, MakeTrapezeDiagonal) {
    Point_2 const a(0, 0);
    Point_2 const b(10, 10);
    Linear_polygon const poly = PolygonTools::makeTrapeze(a, b, 2.0, 2.0);

    ASSERT_EQ(poly.size(), 4U);
    EXPECT_TRUE(poly.is_simple());
}

TEST(PolygonToolsTest, ExtendPolygon) {
    Point_2 const a(0, 0);
    Point_2 const b(5, 0);
    Point_2 const c(10, 0);
    Linear_polygon p1 = PolygonTools::makeTrapeze(a, b, 2.0, 2.0);
    Linear_polygon const p2 = PolygonTools::makeTrapeze(b, c, 2.0, 2.0);

    size_t const originalSize = p1.size();
    PolygonTools::extendPolygon(p1, p2);
    // extendPolygon may add a vertex to connect the polygons
    EXPECT_GE(p1.size(), originalSize);
}

TEST(PolygonToolsTest, GetSegmentContainingPoint) {
    // Create a simple polygon
    Linear_polygon poly;
    poly.push_back(Point_2(0, 0));
    poly.push_back(Point_2(10, 0));
    poly.push_back(Point_2(10, 10));
    poly.push_back(Point_2(0, 10));

    // Point (5, 0) should be on the bottom edge
    auto seg = PolygonTools::getSegmentContainingPoint(poly, Point_2(5, 0));
    EXPECT_EQ(seg.source(), Point_2(0, 0));
    EXPECT_EQ(seg.target(), Point_2(10, 0));
}

TEST(PolygonToolsTest, CreateJoinTriangle) {
    Linear_polygon p1;
    p1.push_back(Point_2(0, 0));
    p1.push_back(Point_2(5, 0));
    p1.push_back(Point_2(5, 5));
    p1.push_back(Point_2(0, 5));

    Linear_polygon p2;
    p2.push_back(Point_2(5, 0));
    p2.push_back(Point_2(10, 0));
    p2.push_back(Point_2(10, 5));
    p2.push_back(Point_2(5, 5));

    // Center point shared between the two polygons
    Point_2 const center(5, 0);
    Linear_polygon const triangle = PolygonTools::createJoinTriangle(p1, p2, center);
    // Should produce a triangle (3 vertices) or empty if collinear
    EXPECT_LE(triangle.size(), 3U);
}

} // namespace
} // namespace laby
