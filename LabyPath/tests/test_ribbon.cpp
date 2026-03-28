/**
 * @file test_ribbon.cpp
 * @brief Unit tests for Ribbon class
 */

#include "Polyline.h"
#include "Ribbon.h"
#include <gtest/gtest.h>

namespace laby {
namespace {

constexpr int kDefaultColor = 0;
constexpr double kDefaultStrokeWidth = 1.0;
constexpr int kFillColor = 5;
constexpr int kAssignedFillColor = 42;
constexpr int kAssignedStrokeColor = 7;
constexpr double kAssignedStrokeWidth = 2.5;
constexpr int kFirstPolylineId = 1;
constexpr int kSecondPolylineId = 2;
constexpr int kMutablePolylineId = 1;
constexpr int kDegeneratePolylineId = 7;
constexpr int kHorizontalOffset = 10;
constexpr int kVerticalOffset = 10;
constexpr int kDiagonalOffset = 5;
constexpr int kConstructedFillColor = 1;
constexpr std::size_t kTwoEntries = 2U;
constexpr std::size_t kSingleEntry = 1U;
constexpr unsigned int kIndexRangeMin = 3U;
constexpr unsigned int kIndexRangeMax = 10U;

TEST(RibbonTest, DefaultConstruction) {
    Ribbon ribbon;
    EXPECT_EQ(ribbon.fillColor(), kDefaultColor);
    EXPECT_TRUE(ribbon.lines().empty());
    EXPECT_EQ(ribbon.strokeColor(), kDefaultColor);
    EXPECT_DOUBLE_EQ(ribbon.strokeWidth(), kDefaultStrokeWidth);
}

TEST(RibbonTest, ConstructWithFillColor) {
    Ribbon const ribbon(kFillColor);
    EXPECT_EQ(ribbon.fillColor(), kFillColor);
}

TEST(RibbonTest, ConstructWithLines) {
    Polyline firstPolyline(kFirstPolylineId);
    firstPolyline.points().emplace_back(0, 0);
    firstPolyline.points().emplace_back(kHorizontalOffset, 0);

    Polyline secondPolyline(kSecondPolylineId);
    secondPolyline.points().emplace_back(0, kVerticalOffset);
    secondPolyline.points().emplace_back(kHorizontalOffset, kVerticalOffset);

    Ribbon const ribbon(kConstructedFillColor, {firstPolyline, secondPolyline});
    EXPECT_EQ(ribbon.lines().size(), kTwoEntries);
    EXPECT_EQ(ribbon.fillColor(), kConstructedFillColor);
}

TEST(RibbonTest, FillColorGetSet) {
    Ribbon ribbon;
    ribbon.setFillColor(kAssignedFillColor);
    EXPECT_EQ(ribbon.fillColor(), kAssignedFillColor);
}

TEST(RibbonTest, StrokeColorGetSet) {
    Ribbon ribbon;
    ribbon.setStrokeColor(kAssignedStrokeColor);
    EXPECT_EQ(ribbon.strokeColor(), kAssignedStrokeColor);
}

TEST(RibbonTest, StrokeWidthGetSet) {
    Ribbon ribbon;
    ribbon.setStrokeWidth(kAssignedStrokeWidth);
    EXPECT_DOUBLE_EQ(ribbon.strokeWidth(), kAssignedStrokeWidth);
}

TEST(RibbonTest, MutableLinesAccess) {
    Ribbon ribbon;
    Polyline polyline(kMutablePolylineId);
    polyline.points().emplace_back(0, 0);
    polyline.points().emplace_back(kDiagonalOffset, kDiagonalOffset);

    ribbon.lines().push_back(polyline);
    EXPECT_EQ(ribbon.lines().size(), kSingleEntry);
    EXPECT_EQ(ribbon.lines()[0].id(), kMutablePolylineId);
}

TEST(RibbonTest, GetSegments) {
    Polyline polyline;
    polyline.points().emplace_back(0, 0);
    polyline.points().emplace_back(kHorizontalOffset, 0);
    polyline.points().emplace_back(kHorizontalOffset, kVerticalOffset);

    Ribbon const ribbon(kDefaultColor, {polyline});
    auto const segments = ribbon.getSegments();
    EXPECT_EQ(segments.size(), kTwoEntries);
}

TEST(RibbonTest, CreateArrSkipsDegenerateSegments) {
    Polyline polyline;
    polyline.setId(kDegeneratePolylineId);
    polyline.points().emplace_back(0, 0);
    polyline.points().emplace_back(0, 0);
    polyline.points().emplace_back(kHorizontalOffset, 0);

    Ribbon const ribbon(kDefaultColor, {polyline});

    EXPECT_NO_THROW({
        auto arrangement = ribbon.createArr();
        EXPECT_EQ(arrangement.number_of_edges(), kSingleEntry);
    });
}

TEST(RibbonTest, GetPoints) {
    Polyline polyline;
    polyline.points().emplace_back(0, 0);
    polyline.points().emplace_back(kDiagonalOffset, kDiagonalOffset);

    Ribbon const ribbon(kDefaultColor, {polyline});
    auto const points = ribbon.getPoints();
    EXPECT_EQ(points.size(), kTwoEntries);
}

TEST(IndexRangeTest, Construction) {
    IndexRange const range{kIndexRangeMin, kIndexRangeMax};
    EXPECT_EQ(range.min, kIndexRangeMin);
    EXPECT_EQ(range.max, kIndexRangeMax);
}

} // namespace
} // namespace laby
