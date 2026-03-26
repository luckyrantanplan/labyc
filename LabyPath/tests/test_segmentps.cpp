/**
 * @file test_segmentps.cpp
 * @brief Unit tests for SegmentPS segment comparison
 */

#include <gtest/gtest.h>
#include "SegmentPS.h"

namespace laby {
namespace {

TEST(SegmentPSTest, ComparisonBySquaredLength) {
    // Create a point set and insert points
    CGAL::Point_set_2<Kernel> ps;
    ps.insert(Point_2(0, 0));
    ps.insert(Point_2(1, 0));
    ps.insert(Point_2(10, 0));

    // Get vertex handles via nearest neighbor search
    auto v0 = ps.nearest_neighbor(Point_2(0, 0));
    auto v1 = ps.nearest_neighbor(Point_2(1, 0));
    auto v2 = ps.nearest_neighbor(Point_2(10, 0));

    SegmentPS short_seg(&*v0, &*v1);  // length^2 = 1
    SegmentPS long_seg(&*v0, &*v2);   // length^2 = 100

    EXPECT_TRUE(short_seg < long_seg);
    EXPECT_FALSE(long_seg < short_seg);
}

TEST(SegmentPSTest, EqualLengthNotLess) {
    CGAL::Point_set_2<Kernel> ps;
    ps.insert(Point_2(0, 0));
    ps.insert(Point_2(3, 4));
    ps.insert(Point_2(5, 0));

    auto v0 = ps.nearest_neighbor(Point_2(0, 0));
    auto v1 = ps.nearest_neighbor(Point_2(3, 4));   // dist^2 = 25
    auto v2 = ps.nearest_neighbor(Point_2(5, 0));   // dist^2 = 25

    SegmentPS seg1(&*v0, &*v1);
    SegmentPS seg2(&*v0, &*v2);

    // Equal squared length: neither should be less
    EXPECT_FALSE(seg1 < seg2);
    EXPECT_FALSE(seg2 < seg1);
}

} // namespace
} // namespace laby
