#include "Anisotrop/Net.h"
#include "GeomData.h"

#include <cstddef>
#include <CGAL/number_utils.h>
#include <cstdint>

#include <gtest/gtest.h>

using namespace laby;
using namespace laby::aniso;

namespace {

constexpr double kCoordZero = 0.0;
constexpr double kCoordOne = 1.0;
constexpr double kCoordTwo = 2.0;
constexpr double kCoordThree = 3.0;
constexpr double kCoordFour = 4.0;
constexpr double kCoordFive = 5.0;
constexpr double kCoordTen = 10.0;

constexpr double kThicknessZero = 0.0;
constexpr double kThicknessOne = 1.0;
constexpr double kThicknessTwo = 2.0;
constexpr double kThicknessThree = 3.0;
constexpr double kThicknessThreePointFive = 3.5;

constexpr std::size_t kPolyConvexIndex = 42U;
constexpr int32_t kNetIdZero = 0;
constexpr int32_t kNetIdOne = 1;
constexpr int32_t kNetIdSeven = 7;

constexpr std::size_t kPathNodeTen = 10U;
constexpr std::size_t kPathNodeTwenty = 20U;
constexpr std::size_t kPathNodeThirty = 30U;
constexpr std::size_t kExpectedPathSize = 3U;

class ArrangementFixture : public ::testing::Test {
  protected:
    auto makeVertex(double xCoord, double yCoord) -> Vertex& {
        auto vertexHandle = _arrangement.insert_in_face_interior(Point_2(xCoord, yCoord),
                                                                 _arrangement.unbounded_face());
        return *vertexHandle;
    }

  private:
    Arrangement_2 _arrangement;
};

class PinTest : public ArrangementFixture {};
class NetTest : public ArrangementFixture {};

} // namespace

TEST_F(PinTest, ConstructionAndThickness) {
    Vertex& vertex = makeVertex(kCoordOne, kCoordTwo);
    Pin const pin(vertex, kThicknessThreePointFive);
    EXPECT_DOUBLE_EQ(pin.thickness(), kThicknessThreePointFive);
}

TEST_F(PinTest, VertexAccessMutable) {
    Vertex& vertex = makeVertex(kCoordFive, kCoordTen);
    Pin pin(vertex, kThicknessOne);
    EXPECT_DOUBLE_EQ(CGAL::to_double(pin.vertex().point().x()), kCoordFive);
    EXPECT_DOUBLE_EQ(CGAL::to_double(pin.vertex().point().y()), kCoordTen);
}

TEST_F(PinTest, VertexAccessConst) {
    Vertex& vertex = makeVertex(kCoordFive, kCoordTen);
    const Pin pin(vertex, kThicknessOne);
    EXPECT_DOUBLE_EQ(CGAL::to_double(pin.vertex().point().x()), kCoordFive);
}

TEST_F(PinTest, PolyConvexIndex) {
    Vertex& vertex = makeVertex(kCoordZero, kCoordZero);
    Pin pin(vertex, kThicknessOne);
    EXPECT_EQ(pin.polyConvexIndex(), 0U);
    pin.setPolyConvexIndex(kPolyConvexIndex);
    EXPECT_EQ(pin.polyConvexIndex(), kPolyConvexIndex);
}

TEST_F(PinTest, DefaultThicknessIsZero) {
    Vertex& vertex = makeVertex(kCoordZero, kCoordZero);
    Pin const pin(vertex, kThicknessZero);
    EXPECT_DOUBLE_EQ(pin.thickness(), kThicknessZero);
}

TEST_F(NetTest, ConstructionAndId) {
    Vertex& sourceVertex = makeVertex(kCoordZero, kCoordZero);
    Vertex& targetVertex = makeVertex(kCoordTen, kCoordTen);
    Pin const src(sourceVertex, kThicknessTwo);
    Pin const tgt(targetVertex, kThicknessThree);

    Net const net(Net::SourcePin{src}, Net::TargetPin{tgt}, kNetIdSeven);
    EXPECT_EQ(net.id(), kNetIdSeven);
}

TEST_F(NetTest, DefaultIdIsZero) {
    Vertex& sourceVertex = makeVertex(kCoordZero, kCoordZero);
    Vertex& targetVertex = makeVertex(kCoordTen, kCoordTen);
    Pin const src(sourceVertex, kThicknessOne);
    Pin const tgt(targetVertex, kThicknessOne);

    Net const net(Net::SourcePin{src}, Net::TargetPin{tgt});
    EXPECT_EQ(net.id(), kNetIdZero);
}

TEST_F(NetTest, SourceAndTarget) {
    Vertex& sourceVertex = makeVertex(kCoordOne, kCoordTwo);
    Vertex& targetVertex = makeVertex(kCoordThree, kCoordFour);
    Pin const src(sourceVertex, kThicknessOne);
    Pin const tgt(targetVertex, kThicknessTwo);

    Net net(Net::SourcePin{src}, Net::TargetPin{tgt}, kNetIdOne);
    EXPECT_DOUBLE_EQ(net.source().thickness(), kThicknessOne);
    EXPECT_DOUBLE_EQ(net.target().thickness(), kThicknessTwo);
}

TEST_F(NetTest, ConstSourceAndTarget) {
    Vertex& sourceVertex = makeVertex(kCoordZero, kCoordZero);
    Vertex& targetVertex = makeVertex(kCoordFive, kCoordFive);
    Pin const src(sourceVertex, kThicknessOne);
    Pin const tgt(targetVertex, kThicknessTwo);

    const Net net(Net::SourcePin{src}, Net::TargetPin{tgt}, kNetIdOne);
    EXPECT_DOUBLE_EQ(net.source().thickness(), kThicknessOne);
    EXPECT_DOUBLE_EQ(net.target().thickness(), kThicknessTwo);
}

TEST_F(NetTest, PathIsEmptyByDefault) {
    Vertex& sourceVertex = makeVertex(kCoordZero, kCoordZero);
    Vertex& targetVertex = makeVertex(kCoordFive, kCoordFive);
    Pin const src(sourceVertex, kThicknessOne);
    Pin const tgt(targetVertex, kThicknessOne);

    Net net(Net::SourcePin{src}, Net::TargetPin{tgt});
    EXPECT_TRUE(net.path().empty());
}

TEST_F(NetTest, PathMutable) {
    Vertex& sourceVertex = makeVertex(kCoordZero, kCoordZero);
    Vertex& targetVertex = makeVertex(kCoordFive, kCoordFive);
    Pin const src(sourceVertex, kThicknessOne);
    Pin const tgt(targetVertex, kThicknessOne);

    Net net(Net::SourcePin{src}, Net::TargetPin{tgt});
    net.path().push_back(kPathNodeTen);
    net.path().push_back(kPathNodeTwenty);
    net.path().push_back(kPathNodeThirty);
    EXPECT_EQ(net.path().size(), kExpectedPathSize);
    EXPECT_EQ(net.path()[1], kPathNodeTwenty);
}

TEST_F(NetTest, IsPlacedDefault) {
    Vertex& sourceVertex = makeVertex(kCoordZero, kCoordZero);
    Vertex& targetVertex = makeVertex(kCoordFive, kCoordFive);
    Pin const src(sourceVertex, kThicknessOne);
    Pin const tgt(targetVertex, kThicknessOne);

    Net const net(Net::SourcePin{src}, Net::TargetPin{tgt});
    EXPECT_FALSE(net.isPlaced());
}

TEST_F(NetTest, MarkPlaced) {
    Vertex& sourceVertex = makeVertex(kCoordZero, kCoordZero);
    Vertex& targetVertex = makeVertex(kCoordFive, kCoordFive);
    Pin const src(sourceVertex, kThicknessOne);
    Pin const tgt(targetVertex, kThicknessOne);

    Net net(Net::SourcePin{src}, Net::TargetPin{tgt});
    net.markPlaced();
    EXPECT_TRUE(net.isPlaced());
}
