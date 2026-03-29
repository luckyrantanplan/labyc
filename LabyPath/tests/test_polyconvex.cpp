/**
 * @file test_polyconvex.cpp
 * @brief Unit tests for PolyConvex: construction, adjacency, intersection,
 *        connect, and geometric validation of generated trapezes.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

#include "GeomData.h"
#include "PolyConvex.h"
#include "basic/PolygonTools.h"
#include "flatteningOverlap/Node.h"

using namespace laby;

// ─── Helpers ────────────────────────────────────────────────────────────────

namespace {

constexpr double kCoordZero = 0.0;
constexpr double kCoordOne = 1.0;
constexpr double kCoordFive = 5.0;
constexpr double kCoordTen = 10.0;
constexpr double kCoordTwenty = 20.0;
constexpr double kCoordHundred = 100.0;
constexpr std::size_t kPolyConvexIdZero = 0U;
constexpr std::size_t kPolyConvexIdOne = 1U;
constexpr std::size_t kPolyConvexIdFortyTwo = 42U;
constexpr std::size_t kAdjacentValueTen = 10U;
constexpr std::size_t kAdjacentValueTwenty = 20U;
constexpr std::size_t kAdjacentValueThirty = 30U;
constexpr std::size_t kPrintedIdSeven = 7U;

/// Create a simple square polygon from (x,y) to (x+s, y+s).
auto makeSquare(double xCoordinate, double yCoordinate, double sideLength) -> Linear_polygon {
    Linear_polygon polygon;
    polygon.push_back(Point_2(xCoordinate, yCoordinate));
    polygon.push_back(Point_2(xCoordinate + sideLength, yCoordinate));
    polygon.push_back(Point_2(xCoordinate + sideLength, yCoordinate + sideLength));
    polygon.push_back(Point_2(xCoordinate, yCoordinate + sideLength));
    return polygon;
}

} // namespace

// ─── Default Construction ───────────────────────────────────────────────────

TEST(PolyConvexTest, DefaultConstruction) {
    const PolyConvex polyConvex;
    EXPECT_TRUE(polyConvex.empty());
    EXPECT_EQ(polyConvex._id, kPolyConvexIdZero);
    EXPECT_EQ(polyConvex._visited, 0);
    EXPECT_TRUE(polyConvex._adjacents.empty());
    EXPECT_DOUBLE_EQ(polyConvex.thickness(), kCoordZero);
}

// ─── Construction with Geometry ─────────────────────────────────────────────

TEST(PolyConvexTest, ConstructFromPointsAndGeometry) {
    const Point_2 sourcePoint(kCoordZero, kCoordZero);
    const Point_2 targetPoint(kCoordTen, kCoordZero);
    const Linear_polygon geometry = makeSquare(kCoordZero, -kCoordOne, kCoordTen);

    const PolyConvex polyConvex(PolyConvexEndpoints{sourcePoint, targetPoint},
                                kPolyConvexIdFortyTwo, geometry);
    EXPECT_EQ(polyConvex._id, kPolyConvexIdFortyTwo);
    EXPECT_FALSE(polyConvex.empty());
    EXPECT_TRUE(polyConvex.hasPoints());
    EXPECT_EQ(polyConvex.getSourcePoint(), sourcePoint);
    EXPECT_EQ(polyConvex.getTargetPoint(), targetPoint);
}

// ─── empty() is const ───────────────────────────────────────────────────────

TEST(PolyConvexTest, EmptyIsConst) {
    const PolyConvex polyConvex;
    EXPECT_TRUE(polyConvex.empty());
}

// ─── Adjacency ──────────────────────────────────────────────────────────────

TEST(PolyConvexTest, ConnectTwoPolygons) {
    std::vector<PolyConvex> polyConvexList;
    polyConvexList.emplace_back();
    polyConvexList.back()._id = kPolyConvexIdZero;
    polyConvexList.back()._geometry = makeSquare(kCoordZero, kCoordZero, kCoordFive);
    polyConvexList.emplace_back();
    polyConvexList.back()._id = kPolyConvexIdOne;
    polyConvexList.back()._geometry = makeSquare(kCoordFive, kCoordZero, kCoordFive);

    PolyConvex::connect({kPolyConvexIdZero, kPolyConvexIdOne}, polyConvexList);

    EXPECT_EQ(polyConvexList[0]._adjacents.size(), 1U);
    EXPECT_EQ(polyConvexList[0]._adjacents[0], kPolyConvexIdOne);
    EXPECT_EQ(polyConvexList[1]._adjacents.size(), 1U);
    EXPECT_EQ(polyConvexList[1]._adjacents[0], kPolyConvexIdZero);
}

TEST(PolyConvexTest, ConnectChain) {
    constexpr std::size_t kChainLength = 4U;

    std::vector<PolyConvex> polyConvexList;
    for (std::size_t index = 0; index < kChainLength; ++index) {
        polyConvexList.emplace_back();
        polyConvexList.back()._id = index;
        polyConvexList.back()._geometry =
            makeSquare(static_cast<double>(index) * kCoordFive, kCoordZero, kCoordFive);
        polyConvexList.back()._originalTrapeze = polyConvexList.back()._geometry;
    }

    PolyConvex::connect(kPolyConvexIdZero, polyConvexList);

    EXPECT_EQ(polyConvexList[0]._adjacents.size(), 1U);
    EXPECT_GE(polyConvexList[1]._adjacents.size(), 1U);
    EXPECT_GE(polyConvexList[2]._adjacents.size(), 1U);
    EXPECT_EQ(polyConvexList[3]._adjacents.size(), 1U);
}

TEST(PolyConvexTest, RemoveAdjacence) {
    PolyConvex polyConvex;
    polyConvex._adjacents = {kAdjacentValueTen, kAdjacentValueTwenty, kAdjacentValueThirty};
    polyConvex.removeAdjacence(kAdjacentValueTwenty);
    EXPECT_EQ(polyConvex._adjacents.size(), 2U);
    EXPECT_TRUE(std::find(polyConvex._adjacents.begin(), polyConvex._adjacents.end(),
                          kAdjacentValueTwenty) == polyConvex._adjacents.end());
}

// ─── Convex Intersection Test ───────────────────────────────────────────────

TEST(PolyConvexTest, OverlappingSquaresIntersect) {
    const Linear_polygon firstSquare = makeSquare(kCoordZero, kCoordZero, kCoordTen);
    const Linear_polygon secondSquare = makeSquare(kCoordFive, kCoordFive, kCoordTen);

    EXPECT_TRUE(PolyConvex::testConvexPolyIntersect(firstSquare, secondSquare));
}

TEST(PolyConvexTest, DisjointSquaresDoNotIntersect) {
    const Linear_polygon firstSquare = makeSquare(kCoordZero, kCoordZero, kCoordFive);
    const Linear_polygon secondSquare = makeSquare(kCoordHundred, kCoordHundred, kCoordFive);

    EXPECT_FALSE(PolyConvex::testConvexPolyIntersect(firstSquare, secondSquare));
}

TEST(PolyConvexTest, ContainedSquareIntersects) {
    const Linear_polygon outerSquare = makeSquare(kCoordZero, kCoordZero, kCoordTwenty);
    const Linear_polygon innerSquare = makeSquare(kCoordFive, kCoordFive, kCoordFive);

    EXPECT_TRUE(PolyConvex::testConvexPolyIntersect(outerSquare, innerSquare));
    EXPECT_TRUE(PolyConvex::testConvexPolyIntersect(innerSquare, outerSquare));
}

TEST(PolyConvexTest, TouchingEdgesIntersect) {
    const Linear_polygon firstSquare = makeSquare(kCoordZero, kCoordZero, kCoordTen);
    const Linear_polygon secondSquare = makeSquare(kCoordTen, kCoordZero, kCoordTen);

    EXPECT_TRUE(PolyConvex::testConvexPolyIntersect(firstSquare, secondSquare));
}

// ─── Mutable Reset ──────────────────────────────────────────────────────────

TEST(PolyConvexTest, ResetMutable) {
    PolyConvex polyConvex;
    polyConvex._visited = -1;
    Node node(0);
    polyConvex._nodes.push_back(&node);

    polyConvex.resetMutable();

    EXPECT_EQ(polyConvex._visited, 0);
    EXPECT_TRUE(polyConvex._nodes.empty());
}

// ─── Clear ──────────────────────────────────────────────────────────────────

TEST(PolyConvexTest, Clear) {
    PolyConvex polyConvex;
    polyConvex._geometry = makeSquare(kCoordZero, kCoordZero, kCoordTen);
    polyConvex._id = kPolyConvexIdFortyTwo;
    polyConvex.setAverageThickness(kCoordFive);

    polyConvex.clear();

    EXPECT_TRUE(polyConvex.empty());
    EXPECT_EQ(polyConvex._id, kPolyConvexIdZero);
    EXPECT_DOUBLE_EQ(polyConvex.thickness(), kCoordZero);
}

// ─── Print ──────────────────────────────────────────────────────────────────

TEST(PolyConvexTest, PrintContainsId) {
    PolyConvex polyConvex;
    polyConvex._id = kPrintedIdSeven;
    polyConvex._geometry = makeSquare(kCoordZero, kCoordZero, kCoordOne);

    std::ostringstream oss;
    polyConvex.print(oss);
    const std::string output = oss.str();

    EXPECT_NE(output.find("id"), std::string::npos);
    EXPECT_NE(output.find('7'), std::string::npos);
}

// ─── Node Containment ───────────────────────────────────────────────────────

TEST(PolyConvexTest, ContainsNode) {
    PolyConvex polyConvex;
    Node firstNode(1);
    const Node secondNode(2);
    polyConvex._nodes.push_back(&firstNode);

    EXPECT_TRUE(polyConvex.contains(firstNode));
    EXPECT_FALSE(polyConvex.contains(secondNode));
}
