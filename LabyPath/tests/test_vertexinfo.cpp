/**
 * @file test_vertexinfo.cpp
 * @brief Unit tests for VertexInfo class
 */

#include <gtest/gtest.h>
#include <complex>
#include "GeomData.h"

namespace laby {
namespace {

TEST(VertexInfoTest, DefaultConstruction) {
    VertexInfo info;
    EXPECT_EQ(info.getType(), VertexInfo::UNDEFINED);
    EXPECT_EQ(info.getVisit(), -1);
    EXPECT_FALSE(info.getGlobalCoordinate().has_value());
    EXPECT_FALSE(info.getDetail().has_value());
    EXPECT_EQ(info.id(), 0);
}

TEST(VertexInfoTest, TypeGetSet) {
    VertexInfo info;
    info.setType(VertexInfo::PIN);
    EXPECT_EQ(info.getType(), VertexInfo::PIN);

    info.setType(VertexInfo::STEINER);
    EXPECT_EQ(info.getType(), VertexInfo::STEINER);
}

TEST(VertexInfoTest, GlobalCoordinate) {
    VertexInfo info;
    std::complex<int32_t> coord(3, 4);
    info.setGlobalCoordinate(coord);

    EXPECT_TRUE(info.getGlobalCoordinate().has_value());
    EXPECT_EQ(info.getGlobalCoordinate().value(), coord);
}

TEST(VertexInfoTest, Detail) {
    VertexInfo info;
    std::complex<int32_t> detail(10, 20);
    info.setDetail(detail);

    EXPECT_TRUE(info.getDetail().has_value());
    EXPECT_EQ(info.getDetail().value(), detail);
}

TEST(VertexInfoTest, IdGetSet) {
    VertexInfo info;
    info.setId(99);
    EXPECT_EQ(info.id(), 99);
}

TEST(VertexInfoTest, TypeEnumValues) {
    EXPECT_EQ(static_cast<int>(VertexInfo::UNDEFINED), 0);
    EXPECT_EQ(static_cast<int>(VertexInfo::STEINER), 1);
    EXPECT_EQ(static_cast<int>(VertexInfo::PIN), 2);
}

} // namespace
} // namespace laby
