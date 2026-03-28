/**
 * @file test_edgeinfo.cpp
 * @brief Unit tests for EdgeInfo class
 */

#include <gtest/gtest.h>
#include "GeomData.h"

namespace laby {
namespace {

TEST(EdgeInfoTest, DefaultConstruction) {
    EdgeInfo const info;
    EXPECT_EQ(info.direction(), -1);
    EXPECT_EQ(info.congestion(), 0);
    EXPECT_DOUBLE_EQ(info.thickness(), 1.0);
    EXPECT_EQ(info.getVisit(), -1);
    EXPECT_EQ(info.coordinate(), 0U);
}

TEST(EdgeInfoTest, ConstructWithDirectionAndCoordinate) {
    EdgeInfo const info(42, EdgeInfo::Coordinate{7});
    EXPECT_EQ(info.direction(), 42);
    EXPECT_EQ(info.coordinate(), 7U);
}

TEST(EdgeInfoTest, ConstructWithType) {
    EdgeInfo const info(EdgeInfo::HORIZONTAL);
    EXPECT_EQ(info.direction(), EdgeInfo::HORIZONTAL);
}

TEST(EdgeInfoTest, AddAndClearPaths) {
    EdgeInfo info;
    EXPECT_EQ(info.congestion(), 0);

    info.addPath(1);
    EXPECT_EQ(info.congestion(), 1);
    EXPECT_TRUE(info.hasNet(1));
    EXPECT_FALSE(info.hasNet(2));

    info.addPath(2);
    EXPECT_EQ(info.congestion(), 2);
    EXPECT_TRUE(info.hasNet(2));

    // Adding same net again should not increase unique net count
    info.addPath(1);
    EXPECT_EQ(info.congestion(), 2);

    info.clearAllPath();
    EXPECT_EQ(info.congestion(), 0);
    EXPECT_FALSE(info.hasNet(1));
}

TEST(EdgeInfoTest, VisitTracking) {
    EdgeInfo const info;
    EXPECT_EQ(info.getVisit(), -1);

    info.setVisit(5);
    EXPECT_EQ(info.getVisit(), 5);
}

TEST(EdgeInfoTest, ThicknessGetSet) {
    EdgeInfo info;
    EXPECT_DOUBLE_EQ(info.thickness(), 1.0);

    info.setThickness(2.5);
    EXPECT_DOUBLE_EQ(info.thickness(), 2.5);

    info.setThickness();
    EXPECT_DOUBLE_EQ(info.thickness(), 1.0);
}

TEST(EdgeInfoTest, Equality) {
    EdgeInfo const a(EdgeInfo::HORIZONTAL);
    EdgeInfo const b(EdgeInfo::HORIZONTAL);
    EdgeInfo const c(EdgeInfo::VERTICAL);

    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
}

TEST(EdgeInfoTest, GetNetReturnsFirstPath) {
    EdgeInfo info;
    info.addPath(42);
    EXPECT_EQ(info.getNet(), 42);
}

TEST(EdgeInfoTest, AllTypeValues) {
    EXPECT_EQ(static_cast<int>(EdgeInfo::UNDEFINED), 1);
    EXPECT_EQ(static_cast<int>(EdgeInfo::HORIZONTAL), 2);
    EXPECT_EQ(static_cast<int>(EdgeInfo::VERTICAL), 3);
    EXPECT_EQ(static_cast<int>(EdgeInfo::DIAGONAL), 4);
    EXPECT_EQ(static_cast<int>(EdgeInfo::SPECIAL), 5);
    EXPECT_EQ(static_cast<int>(EdgeInfo::CELL), 6);
}

} // namespace
} // namespace laby
