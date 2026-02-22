/**
 * @file test_polyline.cpp
 * @brief Unit tests for Polyline class
 */

#include <gtest/gtest.h>
#include "Polyline.h"

namespace laby {
namespace {

TEST(PolylineTest, DefaultConstruction) {
    Polyline pl;
    EXPECT_TRUE(pl.empty());
    EXPECT_EQ(pl.number, 0);
    EXPECT_FALSE(pl.closed);
}

TEST(PolylineTest, ConstructWithNumber) {
    Polyline pl(42);
    EXPECT_TRUE(pl.empty());
    EXPECT_EQ(pl.number, 42);
}

TEST(PolylineTest, ConstructWithPoints) {
    std::vector<Point_2> pts = {Point_2(0, 0), Point_2(1, 0), Point_2(1, 1)};
    Polyline pl(7, pts);
    EXPECT_FALSE(pl.empty());
    EXPECT_EQ(pl.number, 7);
    EXPECT_EQ(pl.points.size(), 3U);
}

TEST(PolylineTest, TotalLengthStraightLine) {
    Polyline pl;
    pl.points.emplace_back(0, 0);
    pl.points.emplace_back(3, 0);
    pl.points.emplace_back(3, 4);

    // Length: 3 + 4 = 7
    EXPECT_NEAR(pl.total_length(), 7.0, 1e-9);
}

TEST(PolylineTest, TotalLengthSinglePoint) {
    Polyline pl;
    pl.points.emplace_back(5, 5);
    EXPECT_DOUBLE_EQ(pl.total_length(), 0.0);
}

TEST(PolylineTest, TotalLengthEmpty) {
    Polyline pl;
    EXPECT_DOUBLE_EQ(pl.total_length(), 0.0);
}

TEST(PolylineTest, ConstructFromPolygon) {
    CGAL::Polygon_2<Kernel> poly;
    poly.push_back(Point_2(0, 0));
    poly.push_back(Point_2(10, 0));
    poly.push_back(Point_2(10, 10));

    Polyline pl(poly);
    EXPECT_TRUE(pl.closed);
    EXPECT_EQ(pl.points.size(), 3U);
}

TEST(PolylineTest, ComputeMinLexi) {
    Polyline pl;
    pl.points.emplace_back(5, 5);
    pl.points.emplace_back(1, 2);
    pl.points.emplace_back(3, 1);

    pl.compute_min_lexi();
    // Lexicographically smallest point: (1, 2)
    EXPECT_EQ(pl.min_point, Point_2(1, 2));
}

} // namespace
} // namespace laby
