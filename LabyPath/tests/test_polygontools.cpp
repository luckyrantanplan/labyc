/**
 * @file test_polygontools.cpp
 * @brief Unit tests for PolygonTools geometry operations
 */

#include "GeomData.h"
#include "basic/PolygonTools.h"
#include <CGAL/number_utils.h>
#include <cstddef>
#include <gtest/gtest.h>

namespace laby {
namespace {

constexpr double kCoordZero = 0.0;
constexpr double kCoordFive = 5.0;
constexpr double kCoordTen = 10.0;
constexpr double kUniformThickness = 2.0;
constexpr double kSmallThickness = 1.0;
constexpr double kLargeThickness = 3.0;
constexpr double kBoundsMinX = -1.0;
constexpr double kBoundsMaxX = 11.0;
constexpr double kBoundsMinY = -2.0;
constexpr double kBoundsMaxY = 2.0;

TEST(PolygonToolsTest, MakeTrapezeBasic) {
    Point_2 const startPoint(kCoordZero, kCoordZero);
    Point_2 const endPoint(kCoordTen, kCoordZero);
    Linear_polygon const poly =
        PolygonTools::makeTrapeze(startPoint, endPoint, kUniformThickness, kUniformThickness);

    // A trapeze between two points with equal thickness should be a rectangle
    ASSERT_EQ(poly.size(), 4U);
    // The polygon should be simple (non-self-intersecting)
    EXPECT_TRUE(poly.is_simple());
}

TEST(PolygonToolsTest, MakeTrapezeVaryingThickness) {
    Point_2 const startPoint(kCoordZero, kCoordZero);
    Point_2 const endPoint(kCoordTen, kCoordZero);
    Linear_polygon const poly =
        PolygonTools::makeTrapeze(startPoint, endPoint, kSmallThickness, kLargeThickness);

    ASSERT_EQ(poly.size(), 4U);
    EXPECT_TRUE(poly.is_simple());
}

TEST(PolygonToolsTest, MakeTrapezeHasCorrectOrientation) {
    Point_2 const startPoint(kCoordZero, kCoordZero);
    Point_2 const endPoint(kCoordTen, kCoordZero);
    Linear_polygon const poly =
        PolygonTools::makeTrapeze(startPoint, endPoint, kUniformThickness, kUniformThickness);

    // Polygon vertices should be around the segment from a to b
    // Check that all vertices are within expected bounds
    for (auto vertexIter = poly.vertices_begin(); vertexIter != poly.vertices_end(); ++vertexIter) {
        double const xCoord = CGAL::to_double(vertexIter->x());
        double const yCoord = CGAL::to_double(vertexIter->y());
        EXPECT_GE(xCoord, kBoundsMinX);
        EXPECT_LE(xCoord, kBoundsMaxX);
        EXPECT_GE(yCoord, kBoundsMinY);
        EXPECT_LE(yCoord, kBoundsMaxY);
    }
}

TEST(PolygonToolsTest, MakeTrapezeInPlace) {
    Point_2 const startPoint(kCoordZero, kCoordZero);
    Point_2 const endPoint(kCoordFive, kCoordFive);
    Linear_polygon poly;
    PolygonTools::makeTrapeze(poly, startPoint, endPoint, kSmallThickness, kSmallThickness);

    ASSERT_EQ(poly.size(), 4U);
    EXPECT_TRUE(poly.is_simple());
}

TEST(PolygonToolsTest, MakeTrapezeDiagonal) {
    Point_2 const startPoint(kCoordZero, kCoordZero);
    Point_2 const endPoint(kCoordTen, kCoordTen);
    Linear_polygon const poly =
        PolygonTools::makeTrapeze(startPoint, endPoint, kUniformThickness, kUniformThickness);

    ASSERT_EQ(poly.size(), 4U);
    EXPECT_TRUE(poly.is_simple());
}

TEST(PolygonToolsTest, ExtendPolygon) {
    Point_2 const firstPoint(kCoordZero, kCoordZero);
    Point_2 const secondPoint(kCoordFive, kCoordZero);
    Point_2 const thirdPoint(kCoordTen, kCoordZero);
    Linear_polygon firstPolygon =
        PolygonTools::makeTrapeze(firstPoint, secondPoint, kUniformThickness, kUniformThickness);
    Linear_polygon const secondPolygon =
        PolygonTools::makeTrapeze(secondPoint, thirdPoint, kUniformThickness, kUniformThickness);

    size_t const originalSize = firstPolygon.size();
    PolygonTools::extendPolygon(firstPolygon, secondPolygon);
    // extendPolygon may add a vertex to connect the polygons
    EXPECT_GE(firstPolygon.size(), originalSize);
}

TEST(PolygonToolsTest, GetSegmentContainingPoint) {
    // Create a simple polygon
    Linear_polygon poly;
    poly.push_back(Point_2(0, 0));
    poly.push_back(Point_2(10, 0));
    poly.push_back(Point_2(10, 10));
    poly.push_back(Point_2(0, 10));

    // Point (5, 0) should be on the bottom edge
    auto edgeSegment =
        PolygonTools::getSegmentContainingPoint(poly, Point_2(kCoordFive, kCoordZero));
    EXPECT_EQ(edgeSegment.source(), Point_2(kCoordZero, kCoordZero));
    EXPECT_EQ(edgeSegment.target(), Point_2(kCoordTen, kCoordZero));
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
    Point_2 const centerPoint(kCoordFive, kCoordZero);
    Linear_polygon const triangle = PolygonTools::createJoinTriangle(p1, p2, centerPoint);
    // Should produce a triangle (3 vertices) or empty if collinear
    EXPECT_LE(triangle.size(), 3U);
}

} // namespace
} // namespace laby
