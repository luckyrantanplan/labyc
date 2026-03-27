/**
 * @file test_ribbon.cpp
 * @brief Unit tests for Ribbon class
 */

#include <gtest/gtest.h>
#include "Ribbon.h"

namespace laby {
namespace {

TEST(RibbonTest, DefaultConstruction) {
    Ribbon r;
    EXPECT_EQ(r.fillColor(), 0);
    EXPECT_TRUE(r.lines().empty());
    EXPECT_EQ(r.strokeColor(), 0);
    EXPECT_DOUBLE_EQ(r.strokeWidth(), 1.0);
}

TEST(RibbonTest, ConstructWithFillColor) {
    Ribbon r(5);
    EXPECT_EQ(r.fillColor(), 5);
}

TEST(RibbonTest, ConstructWithLines) {
    Polyline pl1(1);
    pl1.points.emplace_back(0, 0);
    pl1.points.emplace_back(10, 0);

    Polyline pl2(2);
    pl2.points.emplace_back(0, 10);
    pl2.points.emplace_back(10, 10);

    Ribbon r(1, {pl1, pl2});
    EXPECT_EQ(r.lines().size(), 2U);
    EXPECT_EQ(r.fillColor(), 1);
}

TEST(RibbonTest, FillColorGetSet) {
    Ribbon r;
    r.setFillColor(42);
    EXPECT_EQ(r.fillColor(), 42);
}

TEST(RibbonTest, StrokeColorGetSet) {
    Ribbon r;
    r.setStrokeColor(7);
    EXPECT_EQ(r.strokeColor(), 7);
}

TEST(RibbonTest, StrokeWidthGetSet) {
    Ribbon r;
    r.setStrokeWidth(2.5);
    EXPECT_DOUBLE_EQ(r.strokeWidth(), 2.5);
}

TEST(RibbonTest, MutableLinesAccess) {
    Ribbon r;
    Polyline pl(1);
    pl.points.emplace_back(0, 0);
    pl.points.emplace_back(5, 5);

    r.lines().push_back(pl);
    EXPECT_EQ(r.lines().size(), 1U);
    EXPECT_EQ(r.lines()[0].id, 1);
}

TEST(RibbonTest, GetSegments) {
    Polyline pl;
    pl.points.emplace_back(0, 0);
    pl.points.emplace_back(10, 0);
    pl.points.emplace_back(10, 10);

    Ribbon r(0, {pl});
    auto segs = r.getSegments();
    EXPECT_EQ(segs.size(), 2U);
}

TEST(RibbonTest, CreateArrSkipsDegenerateSegments) {
    Polyline pl;
    pl.id = 7;
    pl.points.emplace_back(0, 0);
    pl.points.emplace_back(0, 0);
    pl.points.emplace_back(10, 0);

    Ribbon r(0, {pl});

    EXPECT_NO_THROW({
        auto arr = r.createArr();
        EXPECT_EQ(arr.number_of_edges(), 1U);
    });
}

TEST(RibbonTest, GetPoints) {
    Polyline pl;
    pl.points.emplace_back(0, 0);
    pl.points.emplace_back(5, 5);

    Ribbon r(0, {pl});
    auto pts = r.getPoints();
    EXPECT_EQ(pts.size(), 2U);
}

TEST(IndexRangeTest, Construction) {
    IndexRange range{3, 10};
    EXPECT_EQ(range.min, 3U);
    EXPECT_EQ(range.max, 10U);
}

} // namespace
} // namespace laby
