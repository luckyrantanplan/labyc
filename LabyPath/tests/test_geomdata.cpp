/**
 * @file test_geomdata.cpp
 * @brief Unit tests for GeomData utility functions and types
 */

#include "GeomData.h"
#include <CGAL/number_utils.h>
#include <complex>
#include <cstdint>
#include <CGAL/Arrangement_2/Arrangement_on_surface_2_global.h>
#include <gtest/gtest.h>
#include <sstream>
#include <vector>
#include <string>

namespace laby {
namespace {

TEST(GeomDataTest, Point2Creation) {
    Point_2 const p(3, 4);
    EXPECT_EQ(CGAL::to_double(p.x()), 3.0);
    EXPECT_EQ(CGAL::to_double(p.y()), 4.0);
}

TEST(GeomDataTest, Segment2Creation) {
    Point_2 const a(0, 0);
    Point_2 const b(1, 1);
    Segment_2 const seg(a, b);
    EXPECT_EQ(seg.source(), a);
    EXPECT_EQ(seg.target(), b);
}

TEST(GeomDataTest, VectorOutput) {
    std::vector<int> const v = {1, 2, 3};
    std::ostringstream oss;
    oss << v;
    std::string const result = oss.str();
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find('1'), std::string::npos);
    EXPECT_NE(result.find('3'), std::string::npos);
}

TEST(GeomDataTest, EmptyVectorOutput) {
    std::vector<int> const v;
    std::ostringstream oss;
    oss << v;
    EXPECT_EQ(oss.str(), "[EMPTY]");
}

TEST(GlobalEdgeTest, Construction) {
    GlobalEdge const edge(
        GlobalEdge::Endpoints{std::complex<int32_t>(1, 2), std::complex<int32_t>(3, 4)});
    EXPECT_EQ(edge.a(), std::complex<int32_t>(1, 2));
    EXPECT_EQ(edge.b(), std::complex<int32_t>(3, 4));
}

TEST(GlobalEdgeTest, Print) {
    GlobalEdge const edge(
        GlobalEdge::Endpoints{std::complex<int32_t>(1, 2), std::complex<int32_t>(3, 4)});
    std::ostringstream oss;
    edge.print(oss);
    std::string const result = oss.str();
    EXPECT_FALSE(result.empty());
}

TEST(ArrangementTest, EmptyArrangement) {
    Arrangement_2 const arr;
    EXPECT_EQ(arr.number_of_vertices(), 0U);
    EXPECT_EQ(arr.number_of_edges(), 0U);
    // An empty arrangement has 1 unbounded face
    EXPECT_EQ(arr.number_of_faces(), 1U);
}

TEST(ArrangementTest, InsertSegment) {
    Arrangement_2 arr;
    Point_2 const p1(0, 0);
    Point_2 const p2(10, 0);
    Segment_info_2 const seg(Segment_2(p1, p2), EdgeInfo(EdgeInfo::HORIZONTAL));
    CGAL::insert(arr, seg);
    EXPECT_EQ(arr.number_of_vertices(), 2U);
    EXPECT_EQ(arr.number_of_edges(), 1U);
}

} // namespace
} // namespace laby
