/**
 * @file test_polyline.cpp
 * @brief Unit tests for Polyline class
 */

#include <CGAL/Polygon_2.h>
#include <gtest/gtest.h>
#include <vector>
#include "GeomData.h"
#include "Polyline.h"

namespace laby {
namespace {

TEST(PolylineTest, DefaultConstruction) {
    Polyline const pl;
    EXPECT_TRUE(pl.empty());
    EXPECT_EQ(pl.id(), 0);
    EXPECT_FALSE(pl.isClosed());
}

TEST(PolylineTest, ConstructWithNumber) {
    Polyline const pl(42);
    EXPECT_TRUE(pl.empty());
    EXPECT_EQ(pl.id(), 42);
}

TEST(PolylineTest, ConstructWithPoints) {
    std::vector<Point_2> const pts = {Point_2(0, 0), Point_2(1, 0), Point_2(1, 1)};
    Polyline const pl(7, pts);
    EXPECT_FALSE(pl.empty());
    EXPECT_EQ(pl.id(), 7);
    EXPECT_EQ(pl.points().size(), 3U);
}

TEST(PolylineTest, TotalLengthStraightLine) {
    Polyline pl;
    pl.points().emplace_back(0, 0);
    pl.points().emplace_back(3, 0);
    pl.points().emplace_back(3, 4);

    // Length: 3 + 4 = 7
    EXPECT_NEAR(pl.totalLength(), 7.0, 1e-9);
}

TEST(PolylineTest, TotalLengthSinglePoint) {
    Polyline pl;
    pl.points().emplace_back(5, 5);
    EXPECT_DOUBLE_EQ(pl.totalLength(), 0.0);
}

TEST(PolylineTest, TotalLengthEmpty) {
    Polyline const pl;
    EXPECT_DOUBLE_EQ(pl.totalLength(), 0.0);
}

TEST(PolylineTest, ConstructFromPolygon) {
    CGAL::Polygon_2<Kernel> poly;
    poly.push_back(Point_2(0, 0));
    poly.push_back(Point_2(10, 0));
    poly.push_back(Point_2(10, 10));

    Polyline const pl(poly);
    EXPECT_TRUE(pl.isClosed());
    EXPECT_EQ(pl.points().size(), 3U);
}

TEST(PolylineTest, ComputeMinLexi) {
    Polyline pl;
    pl.points().emplace_back(5, 5);
    pl.points().emplace_back(1, 2);
    pl.points().emplace_back(3, 1);

    pl.computeMinLexi();
    // Lexicographically smallest point: (1, 2)
    EXPECT_EQ(pl.minPoint(), Point_2(1, 2));
}

TEST(PolylineTest, RemoveConsecutiveDuplicatePoints) {
    Polyline pl;
    pl.points().emplace_back(0, 0);
    pl.points().emplace_back(1, 0);
    pl.points().emplace_back(1, 0);
    pl.points().emplace_back(1, 0);
    pl.points().emplace_back(2, 0);

    pl.removeConsecutiveDuplicatePoints();

    ASSERT_EQ(pl.points().size(), 3U);
    EXPECT_EQ(pl.points().at(0), Point_2(0, 0));
    EXPECT_EQ(pl.points().at(1), Point_2(1, 0));
    EXPECT_EQ(pl.points().at(2), Point_2(2, 0));
}

TEST(PolylineTest, RemoveNearlyConsecutiveDuplicatePoints) {
    Polyline pl;
    pl.points().emplace_back(0, 0);
    pl.points().emplace_back(1, 0);
    pl.points().emplace_back(1.0 + 1e-13, 1e-13);
    pl.points().emplace_back(2, 0);

    pl.removeConsecutiveDuplicatePoints(1e-9);

    ASSERT_EQ(pl.points().size(), 3U);
    EXPECT_EQ(pl.points().at(0), Point_2(0, 0));
    EXPECT_EQ(pl.points().at(1), Point_2(1, 0));
    EXPECT_EQ(pl.points().at(2), Point_2(2, 0));
}

} // namespace
} // namespace laby
