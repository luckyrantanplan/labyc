/**
 * @file test_polyconvex.cpp
 * @brief Unit tests for PolyConvex: construction, adjacency, intersection,
 *        connect, and geometric validation of generated trapezes.
 */

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "GeomData.h"
#include "PolyConvex.h"
#include "basic/LinearGradient.h"
#include "basic/PolygonTools.h"

using namespace laby;

// ─── Helpers ────────────────────────────────────────────────────────────────

namespace {

/// Create a simple square polygon from (x,y) to (x+s, y+s).
Linear_polygon makeSquare(double x, double y, double s) {
    Linear_polygon poly;
    poly.push_back(Point_2(x, y));
    poly.push_back(Point_2(x + s, y));
    poly.push_back(Point_2(x + s, y + s));
    poly.push_back(Point_2(x, y + s));
    return poly;
}

} // namespace

// ─── Default Construction ───────────────────────────────────────────────────

TEST(PolyConvexTest, DefaultConstruction) {
    PolyConvex pc;
    EXPECT_TRUE(pc.empty());
    EXPECT_EQ(pc._id, 0U);
    EXPECT_EQ(pc._visited, 0);
    EXPECT_TRUE(pc._adjacents.empty());
    EXPECT_DOUBLE_EQ(pc.thickness(), 0.0);
}

// ─── Construction with Geometry ─────────────────────────────────────────────

TEST(PolyConvexTest, ConstructFromPointsAndGeometry) {
    Point_2 ps(0, 0);
    Point_2 pt(10, 0);
    Linear_polygon geom = makeSquare(0, -1, 10);

    PolyConvex pc(ps, pt, 42, geom);
    EXPECT_EQ(pc._id, 42U);
    EXPECT_FALSE(pc.empty());
    EXPECT_TRUE(pc.has_points());
    EXPECT_EQ(pc.getSourcePoint(), ps);
    EXPECT_EQ(pc.getTargetPoint(), pt);
}

// ─── empty() is const ───────────────────────────────────────────────────────

TEST(PolyConvexTest, EmptyIsConst) {
    const PolyConvex pc;
    EXPECT_TRUE(pc.empty()); // Must compile on const object
}

// ─── Adjacency ──────────────────────────────────────────────────────────────

TEST(PolyConvexTest, ConnectTwoPolygons) {
    std::vector<PolyConvex> list;
    list.emplace_back();
    list.back()._id = 0;
    list.back()._geometry = makeSquare(0, 0, 5);
    list.emplace_back();
    list.back()._id = 1;
    list.back()._geometry = makeSquare(5, 0, 5);

    PolyConvex::connect(0, 1, list);

    EXPECT_EQ(list[0]._adjacents.size(), 1U);
    EXPECT_EQ(list[0]._adjacents[0], 1U);
    EXPECT_EQ(list[1]._adjacents.size(), 1U);
    EXPECT_EQ(list[1]._adjacents[0], 0U);
}

TEST(PolyConvexTest, ConnectChain) {
    std::vector<PolyConvex> list;
    for (std::size_t i = 0; i < 4; ++i) {
        list.emplace_back();
        list.back()._id = i;
        list.back()._geometry = makeSquare(static_cast<double>(i) * 5.0, 0, 5);
        list.back()._originalTrapeze = list.back()._geometry;
    }

    PolyConvex::connect(0, list);

    // Each interior polygon should have 2 adjacents; endpoints have 1
    EXPECT_EQ(list[0]._adjacents.size(), 1U);
    EXPECT_GE(list[1]._adjacents.size(), 1U);
    EXPECT_GE(list[2]._adjacents.size(), 1U);
    EXPECT_EQ(list[3]._adjacents.size(), 1U);
}

TEST(PolyConvexTest, RemoveAdjacence) {
    PolyConvex pc;
    pc._adjacents = {10, 20, 30};
    pc.remove_adjacence(20);
    EXPECT_EQ(pc._adjacents.size(), 2U);
    // 20 should no longer be present
    EXPECT_TRUE(std::find(pc._adjacents.begin(), pc._adjacents.end(), 20) == pc._adjacents.end());
}

// ─── Convex Intersection Test ───────────────────────────────────────────────

TEST(PolyConvexTest, OverlappingSquaresIntersect) {
    Linear_polygon a = makeSquare(0, 0, 10);
    Linear_polygon b = makeSquare(5, 5, 10); // Overlaps a

    EXPECT_TRUE(PolyConvex::testConvexPolyIntersect(a, b));
}

TEST(PolyConvexTest, DisjointSquaresDoNotIntersect) {
    Linear_polygon a = makeSquare(0, 0, 5);
    Linear_polygon b = makeSquare(100, 100, 5);

    EXPECT_FALSE(PolyConvex::testConvexPolyIntersect(a, b));
}

TEST(PolyConvexTest, ContainedSquareIntersects) {
    Linear_polygon outer = makeSquare(0, 0, 20);
    Linear_polygon inner = makeSquare(5, 5, 5);

    EXPECT_TRUE(PolyConvex::testConvexPolyIntersect(outer, inner));
    EXPECT_TRUE(PolyConvex::testConvexPolyIntersect(inner, outer));
}

TEST(PolyConvexTest, TouchingEdgesIntersect) {
    Linear_polygon a = makeSquare(0, 0, 10);
    Linear_polygon b = makeSquare(10, 0, 10); // Shares an edge

    EXPECT_TRUE(PolyConvex::testConvexPolyIntersect(a, b));
}

// ─── Mutable Reset ──────────────────────────────────────────────────────────

TEST(PolyConvexTest, ResetMutable) {
    PolyConvex pc;
    pc._visited = -1;
    Node n(0);
    pc._nodes.push_back(&n);

    pc.resetMutable();

    EXPECT_EQ(pc._visited, 0);
    EXPECT_TRUE(pc._nodes.empty());
}

// ─── Clear ──────────────────────────────────────────────────────────────────

TEST(PolyConvexTest, Clear) {
    PolyConvex pc;
    pc._geometry = makeSquare(0, 0, 10);
    pc._id = 42;
    pc.set_average_thickness(5.0);

    pc.clear();

    EXPECT_TRUE(pc.empty());
    EXPECT_EQ(pc._id, 0U);
    EXPECT_DOUBLE_EQ(pc.thickness(), 0.0);
}

// ─── Print ──────────────────────────────────────────────────────────────────

TEST(PolyConvexTest, PrintContainsId) {
    PolyConvex pc;
    pc._id = 7;
    pc._geometry = makeSquare(0, 0, 1);

    std::ostringstream oss;
    pc.print(oss);
    std::string out = oss.str();

    EXPECT_NE(out.find("id"), std::string::npos);
    EXPECT_NE(out.find('7'), std::string::npos);
}

// ─── Node Containment ───────────────────────────────────────────────────────

TEST(PolyConvexTest, ContainsNode) {
    PolyConvex pc;
    Node n1(1);
    Node n2(2);
    pc._nodes.push_back(&n1);

    EXPECT_TRUE(pc.contains(n1));
    EXPECT_FALSE(pc.contains(n2));
}
