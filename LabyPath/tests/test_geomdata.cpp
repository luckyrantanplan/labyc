/**
 * @file test_geomdata.cpp
 * @brief Unit tests for GeomData utility functions and types
 */

#include "GeomData.h"
#include <gtest/gtest.h>
#include <sstream>

namespace laby {
namespace {

TEST(GeomDataTest, Point2Creation) {
    Point_2 p(3, 4);
    EXPECT_EQ(CGAL::to_double(p.x()), 3.0);
    EXPECT_EQ(CGAL::to_double(p.y()), 4.0);
}

TEST(GeomDataTest, Segment2Creation) {
    Point_2 a(0, 0);
    Point_2 b(1, 1);
    Segment_2 seg(a, b);
    EXPECT_EQ(seg.source(), a);
    EXPECT_EQ(seg.target(), b);
}

TEST(GeomDataTest, VectorOutput) {
    std::vector<int> v = {1, 2, 3};
    std::ostringstream oss;
    oss << v;
    std::string result = oss.str();
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find('1'), std::string::npos);
    EXPECT_NE(result.find('3'), std::string::npos);
}

TEST(GeomDataTest, EmptyVectorOutput) {
    std::vector<int> v;
    std::ostringstream oss;
    oss << v;
    EXPECT_EQ(oss.str(), "[EMPTY]");
}

TEST(GlobalEdgeTest, Construction) {
    GlobalEdge edge(
        GlobalEdge::Endpoints{std::complex<int32_t>(1, 2), std::complex<int32_t>(3, 4)});
    EXPECT_EQ(edge.a(), std::complex<int32_t>(1, 2));
    EXPECT_EQ(edge.b(), std::complex<int32_t>(3, 4));
}

TEST(GlobalEdgeTest, Print) {
    GlobalEdge edge(
        GlobalEdge::Endpoints{std::complex<int32_t>(1, 2), std::complex<int32_t>(3, 4)});
    std::ostringstream oss;
    edge.print(oss);
    std::string result = oss.str();
    EXPECT_FALSE(result.empty());
}

TEST(ArrangementTest, EmptyArrangement) {
    Arrangement_2 arr;
    EXPECT_EQ(arr.number_of_vertices(), 0U);
    EXPECT_EQ(arr.number_of_edges(), 0U);
    // An empty arrangement has 1 unbounded face
    EXPECT_EQ(arr.number_of_faces(), 1U);
}

TEST(ArrangementTest, InsertSegment) {
    Arrangement_2 arr;
    Point_2 p1(0, 0);
    Point_2 p2(10, 0);
    Segment_info_2 seg(Segment_2(p1, p2), EdgeInfo(EdgeInfo::HORIZONTAL));
    CGAL::insert(arr, seg);
    EXPECT_EQ(arr.number_of_vertices(), 2U);
    EXPECT_EQ(arr.number_of_edges(), 1U);
}

} // namespace
} // namespace laby
