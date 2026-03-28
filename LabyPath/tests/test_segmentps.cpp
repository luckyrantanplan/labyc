/**
 * @file test_segmentps.cpp
 * @brief Unit tests for SegmentPS segment comparison
 */

#include "GeomData.h"
#include "SegmentPS.h"
#include <CGAL/Point_set_2.h>
#include <gtest/gtest.h>

namespace laby {
namespace {

constexpr int kOriginX = 0;
constexpr int kOriginY = 0;
constexpr int kShortSegmentX = 1;
constexpr int kLongSegmentX = 10;
constexpr int kTrianglePointX = 3;
constexpr int kTrianglePointY = 4;
constexpr int kEqualLengthX = 5;

TEST(SegmentPSTest, ComparisonBySquaredLength) {
    // Create a point set and insert points
    CGAL::Point_set_2<Kernel> pointSet;
    pointSet.insert(Point_2(kOriginX, kOriginY));
    pointSet.insert(Point_2(kShortSegmentX, kOriginY));
    pointSet.insert(Point_2(kLongSegmentX, kOriginY));

    // Get vertex handles via nearest neighbor search
    auto originVertex = pointSet.nearest_neighbor(Point_2(kOriginX, kOriginY));
    auto shortVertex = pointSet.nearest_neighbor(Point_2(kShortSegmentX, kOriginY));
    auto longVertex = pointSet.nearest_neighbor(Point_2(kLongSegmentX, kOriginY));

    SegmentPS const shortSegment(&*originVertex, &*shortVertex); // length^2 = 1
    SegmentPS const longSegment(&*originVertex, &*longVertex);   // length^2 = 100

    EXPECT_TRUE(shortSegment < longSegment);
    EXPECT_FALSE(longSegment < shortSegment);
}

TEST(SegmentPSTest, EqualLengthNotLess) {
    CGAL::Point_set_2<Kernel> pointSet;
    pointSet.insert(Point_2(kOriginX, kOriginY));
    pointSet.insert(Point_2(kTrianglePointX, kTrianglePointY));
    pointSet.insert(Point_2(kEqualLengthX, kOriginY));

    auto originVertex = pointSet.nearest_neighbor(Point_2(kOriginX, kOriginY));
    auto diagonalVertex = pointSet.nearest_neighbor(Point_2(kTrianglePointX, kTrianglePointY));
    auto horizontalVertex = pointSet.nearest_neighbor(Point_2(kEqualLengthX, kOriginY));

    SegmentPS const firstSegment(&*originVertex, &*diagonalVertex);
    SegmentPS const secondSegment(&*originVertex, &*horizontalVertex);

    // Equal squared length: neither should be less
    EXPECT_FALSE(firstSegment < secondSegment);
    EXPECT_FALSE(secondSegment < firstSegment);
}

} // namespace
} // namespace laby
