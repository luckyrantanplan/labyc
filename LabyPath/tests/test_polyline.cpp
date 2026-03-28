/**
 * @file test_polyline.cpp
 * @brief Unit tests for Polyline class
 */

#include "GeomData.h"
#include "Polyline.h"
#include <CGAL/Polygon_2.h>
#include <gtest/gtest.h>
#include <vector>

namespace laby {
namespace {

constexpr std::size_t kPointCount = 3U;
constexpr int kDefaultPolylineId = 0;
constexpr int kFirstPolylineId = 42;
constexpr int kSecondPolylineId = 7;
constexpr double kCoordZero = 0.0;
constexpr double kCoordOne = 1.0;
constexpr double kCoordThree = 3.0;
constexpr double kCoordFour = 4.0;
constexpr double kCoordFive = 5.0;
constexpr double kCoordTen = 10.0;
constexpr double kExpectedTotalLength = 7.0;
constexpr double kDuplicateEpsilon = 1e-13;
constexpr double kDuplicateTolerance = 1e-9;

TEST(PolylineTest, DefaultConstruction) {
    Polyline const polyline;
    EXPECT_TRUE(polyline.empty());
    EXPECT_EQ(polyline.id(), kDefaultPolylineId);
    EXPECT_FALSE(polyline.isClosed());
}

TEST(PolylineTest, ConstructWithNumber) {
    Polyline const polyline(kFirstPolylineId);
    EXPECT_TRUE(polyline.empty());
    EXPECT_EQ(polyline.id(), kFirstPolylineId);
}

TEST(PolylineTest, ConstructWithPoints) {
    std::vector<Point_2> const points = {Point_2(kCoordZero, kCoordZero),
                                         Point_2(kCoordOne, kCoordZero),
                                         Point_2(kCoordOne, kCoordOne)};
    Polyline const polyline(kSecondPolylineId, points);
    EXPECT_FALSE(polyline.empty());
    EXPECT_EQ(polyline.id(), kSecondPolylineId);
    EXPECT_EQ(polyline.points().size(), kPointCount);
}

TEST(PolylineTest, TotalLengthStraightLine) {
    Polyline polyline;
    polyline.points().emplace_back(kCoordZero, kCoordZero);
    polyline.points().emplace_back(kCoordThree, kCoordZero);
    polyline.points().emplace_back(kCoordThree, kCoordFour);

    // Length: 3 + 4 = 7
    EXPECT_NEAR(polyline.totalLength(), kExpectedTotalLength, kDuplicateTolerance);
}

TEST(PolylineTest, TotalLengthSinglePoint) {
    Polyline polyline;
    polyline.points().emplace_back(kCoordFive, kCoordFive);
    EXPECT_DOUBLE_EQ(polyline.totalLength(), kCoordZero);
}

TEST(PolylineTest, TotalLengthEmpty) {
    Polyline const polyline;
    EXPECT_DOUBLE_EQ(polyline.totalLength(), kCoordZero);
}

TEST(PolylineTest, ConstructFromPolygon) {
    CGAL::Polygon_2<Kernel> poly;
    poly.push_back(Point_2(0, 0));
    poly.push_back(Point_2(10, 0));
    poly.push_back(Point_2(10, 10));

    Polyline const polyline(poly);
    EXPECT_TRUE(polyline.isClosed());
    EXPECT_EQ(polyline.points().size(), kPointCount);
}

TEST(PolylineTest, ComputeMinLexi) {
    Polyline polyline;
    polyline.points().emplace_back(kCoordFive, kCoordFive);
    polyline.points().emplace_back(kCoordOne, 2);
    polyline.points().emplace_back(kCoordThree, kCoordOne);

    polyline.computeMinLexi();
    // Lexicographically smallest point: (1, 2)
    EXPECT_EQ(polyline.minPoint(), Point_2(kCoordOne, 2));
}

TEST(PolylineTest, RemoveConsecutiveDuplicatePoints) {
    Polyline polyline;
    polyline.points().emplace_back(kCoordZero, kCoordZero);
    polyline.points().emplace_back(kCoordOne, kCoordZero);
    polyline.points().emplace_back(kCoordOne, kCoordZero);
    polyline.points().emplace_back(kCoordOne, kCoordZero);
    polyline.points().emplace_back(2, kCoordZero);

    polyline.removeConsecutiveDuplicatePoints();

    ASSERT_EQ(polyline.points().size(), kPointCount);
    EXPECT_EQ(polyline.points().at(0), Point_2(kCoordZero, kCoordZero));
    EXPECT_EQ(polyline.points().at(1), Point_2(kCoordOne, kCoordZero));
    EXPECT_EQ(polyline.points().at(2), Point_2(2, kCoordZero));
}

TEST(PolylineTest, RemoveNearlyConsecutiveDuplicatePoints) {
    Polyline polyline;
    polyline.points().emplace_back(kCoordZero, kCoordZero);
    polyline.points().emplace_back(kCoordOne, kCoordZero);
    polyline.points().emplace_back(kCoordOne + kDuplicateEpsilon, kDuplicateEpsilon);
    polyline.points().emplace_back(2, kCoordZero);

    polyline.removeConsecutiveDuplicatePoints(kDuplicateTolerance);

    ASSERT_EQ(polyline.points().size(), kPointCount);
    EXPECT_EQ(polyline.points().at(0), Point_2(kCoordZero, kCoordZero));
    EXPECT_EQ(polyline.points().at(1), Point_2(kCoordOne, kCoordZero));
    EXPECT_EQ(polyline.points().at(2), Point_2(2, kCoordZero));
}

} // namespace
} // namespace laby
