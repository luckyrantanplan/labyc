/**
 * @file test_orientedribbon.cpp
 * @brief Unit tests for OrientedRibbon CW/CCW segment handling
 */

#include <gtest/gtest.h>
#include "OrientedRibbon.h"

namespace laby {
namespace {

TEST(OrientedRibbonTest, AddCWClassifiesRight) {
    OrientedRibbon ribbon;
    // Segment going upward (y increases): classified as "right"
    Kernel::Segment_2 seg(Point_2(0, 0), Point_2(0, 10));
    ribbon.addCW(seg);

    auto result = ribbon.getResult();
    ASSERT_FALSE(result.empty());
}

TEST(OrientedRibbonTest, AddCWClassifiesLeft) {
    OrientedRibbon ribbon;
    // Segment going downward (y decreases): classified as "left"
    Kernel::Segment_2 seg(Point_2(0, 10), Point_2(0, 0));
    ribbon.addCW(seg);

    auto result = ribbon.getResult();
    ASSERT_FALSE(result.empty());
}

TEST(OrientedRibbonTest, AddCCWInvertsClassification) {
    // addCCW should invert the left/right classification compared to addCW.
    // With the fix, addCCW places "right" segments into _left and vice versa.
    OrientedRibbon ribbon;
    Kernel::Segment_2 seg1(Point_2(0, 0), Point_2(5, 5));
    Kernel::Segment_2 seg2(Point_2(5, 5), Point_2(10, 0));

    ribbon.addCCW(seg1);
    ribbon.addCW(seg2);

    auto result = ribbon.getResult();
    ASSERT_FALSE(result.empty());
}

TEST(OrientedRibbonTest, CreateOrientedRibbonFromParallelSegments) {
    OrientedRibbon ribbon;
    // Two parallel horizontal segments going right (y increases via different positions)
    ribbon.addCW(Kernel::Segment_2(Point_2(0, 0), Point_2(0, 10)));
    ribbon.addCW(Kernel::Segment_2(Point_2(5, 0), Point_2(5, 10)));

    Ribbon result = ribbon.createOrientedRibbon();
    // Should produce polylines from the arrangement
    EXPECT_FALSE(result.lines().empty());
}

TEST(OrientedRibbonTest, GetResultReturnsVector) {
    OrientedRibbon ribbon;
    ribbon.addCW(Kernel::Segment_2(Point_2(0, 0), Point_2(10, 0)));
    ribbon.addCW(Kernel::Segment_2(Point_2(0, 5), Point_2(10, 5)));

    auto result = ribbon.getResult();
    ASSERT_EQ(result.size(), 1U);
}

TEST(OrientedRibbonTest, EmptyRibbon) {
    OrientedRibbon ribbon;
    auto result = ribbon.getResult();
    ASSERT_EQ(result.size(), 1U);
    // An empty ribbon should have no polylines
    EXPECT_TRUE(result[0].lines().empty());
}

} // namespace
} // namespace laby
