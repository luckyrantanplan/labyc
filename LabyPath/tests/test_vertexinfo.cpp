/**
 * @file test_vertexinfo.cpp
 * @brief Unit tests for VertexInfo class
 */

#include "GeomData.h"
#include <complex>
#include <cstdint>
#include <gtest/gtest.h>
#include <optional>

namespace laby {
namespace {

constexpr int kUnsetVisit = -1;
constexpr int kDefaultId = 0;
constexpr int32_t kCoordinateX = 3;
constexpr int32_t kCoordinateY = 4;
constexpr int32_t kDetailX = 10;
constexpr int32_t kDetailY = 20;
constexpr int kAssignedId = 99;
constexpr int kUndefinedTypeValue = 0;
constexpr int kSteinerTypeValue = 1;
constexpr int kPinTypeValue = 2;

TEST(VertexInfoTest, DefaultConstruction) {
    VertexInfo const info;
    EXPECT_EQ(info.getType(), VertexInfo::UNDEFINED);
    EXPECT_EQ(info.getVisit(), kUnsetVisit);
    EXPECT_FALSE(info.getGlobalCoordinate().has_value());
    EXPECT_FALSE(info.getDetail().has_value());
    EXPECT_EQ(info.id(), kDefaultId);
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
    std::complex<int32_t> const coordinate(kCoordinateX, kCoordinateY);
    info.setGlobalCoordinate(coordinate);

    EXPECT_TRUE(info.getGlobalCoordinate().has_value());
    EXPECT_EQ(info.getGlobalCoordinate(), std::optional<std::complex<int32_t>>(coordinate));
}

TEST(VertexInfoTest, Detail) {
    VertexInfo info;
    std::complex<int32_t> const detailCoordinate(kDetailX, kDetailY);
    info.setDetail(detailCoordinate);

    EXPECT_TRUE(info.getDetail().has_value());
    EXPECT_EQ(info.getDetail(), std::optional<std::complex<int32_t>>(detailCoordinate));
}

TEST(VertexInfoTest, IdGetSet) {
    VertexInfo info;
    info.setId(kAssignedId);
    EXPECT_EQ(info.id(), kAssignedId);
}

TEST(VertexInfoTest, TypeEnumValues) {
    EXPECT_EQ(static_cast<int>(VertexInfo::UNDEFINED), kUndefinedTypeValue);
    EXPECT_EQ(static_cast<int>(VertexInfo::STEINER), kSteinerTypeValue);
    EXPECT_EQ(static_cast<int>(VertexInfo::PIN), kPinTypeValue);
}

} // namespace
} // namespace laby
