#include <gtest/gtest.h>
#include "GeomData.h"
#include "Anisotrop/Net.h"

using namespace laby;
using namespace laby::aniso;

// ─── Pin Tests ──────────────────────────────────────────────────────────────

class PinTest : public ::testing::Test {
protected:
    Arrangement_2 arr;

    // Create a vertex in an arrangement for testing
    Vertex& makeVertex(double x, double y) {
        auto vh = arr.insert_in_face_interior(Point_2(x, y), arr.unbounded_face());
        return *vh;
    }
};

TEST_F(PinTest, ConstructionAndThickness) {
    Vertex& v = makeVertex(1.0, 2.0);
    Pin pin(v, 3.5);
    EXPECT_DOUBLE_EQ(pin.thickness(), 3.5);
}

TEST_F(PinTest, VertexAccessMutable) {
    Vertex& v = makeVertex(5.0, 10.0);
    Pin pin(v, 1.0);
    EXPECT_DOUBLE_EQ(CGAL::to_double(pin.vertex().point().x()), 5.0);
    EXPECT_DOUBLE_EQ(CGAL::to_double(pin.vertex().point().y()), 10.0);
}

TEST_F(PinTest, VertexAccessConst) {
    Vertex& v = makeVertex(5.0, 10.0);
    const Pin pin(v, 1.0);
    EXPECT_DOUBLE_EQ(CGAL::to_double(pin.vertex().point().x()), 5.0);
}

TEST_F(PinTest, PolyConvexIndex) {
    Vertex& v = makeVertex(0, 0);
    Pin pin(v, 1.0);
    EXPECT_EQ(pin.polyConvexIndex(), 0u);  // default
    pin.setPolyConvexIndex(42);
    EXPECT_EQ(pin.polyConvexIndex(), 42u);
}

TEST_F(PinTest, DefaultThicknessIsZero) {
    // Pin always requires explicit thickness
    Vertex& v = makeVertex(0, 0);
    Pin pin(v, 0.0);
    EXPECT_DOUBLE_EQ(pin.thickness(), 0.0);
}

// ─── Net Tests ──────────────────────────────────────────────────────────────

class NetTest : public ::testing::Test {
protected:
    Arrangement_2 arr;

    Vertex& makeVertex(double x, double y) {
        auto vh = arr.insert_in_face_interior(Point_2(x, y), arr.unbounded_face());
        return *vh;
    }
};

TEST_F(NetTest, ConstructionAndId) {
    Vertex& v1 = makeVertex(0, 0);
    Vertex& v2 = makeVertex(10, 10);
    Pin src(v1, 2.0);
    Pin tgt(v2, 3.0);

    Net net(src, tgt, 7);
    EXPECT_EQ(net.id(), 7);
}

TEST_F(NetTest, DefaultIdIsZero) {
    Vertex& v1 = makeVertex(0, 0);
    Vertex& v2 = makeVertex(10, 10);
    Pin src(v1, 1.0);
    Pin tgt(v2, 1.0);

    Net net(src, tgt);
    EXPECT_EQ(net.id(), 0);
}

TEST_F(NetTest, SourceAndTarget) {
    Vertex& v1 = makeVertex(1, 2);
    Vertex& v2 = makeVertex(3, 4);
    Pin src(v1, 1.0);
    Pin tgt(v2, 2.0);

    Net net(src, tgt, 1);
    EXPECT_DOUBLE_EQ(net.source().thickness(), 1.0);
    EXPECT_DOUBLE_EQ(net.target().thickness(), 2.0);
}

TEST_F(NetTest, ConstSourceAndTarget) {
    Vertex& v1 = makeVertex(0, 0);
    Vertex& v2 = makeVertex(5, 5);
    Pin src(v1, 1.0);
    Pin tgt(v2, 2.0);

    const Net net(src, tgt, 1);
    EXPECT_DOUBLE_EQ(net.source().thickness(), 1.0);
    EXPECT_DOUBLE_EQ(net.target().thickness(), 2.0);
}

TEST_F(NetTest, PathIsEmptyByDefault) {
    Vertex& v1 = makeVertex(0, 0);
    Vertex& v2 = makeVertex(5, 5);
    Pin src(v1, 1.0);
    Pin tgt(v2, 1.0);

    Net net(src, tgt);
    EXPECT_TRUE(net.path().empty());
}

TEST_F(NetTest, PathMutable) {
    Vertex& v1 = makeVertex(0, 0);
    Vertex& v2 = makeVertex(5, 5);
    Pin src(v1, 1.0);
    Pin tgt(v2, 1.0);

    Net net(src, tgt);
    net.path().push_back(10);
    net.path().push_back(20);
    net.path().push_back(30);
    EXPECT_EQ(net.path().size(), 3u);
    EXPECT_EQ(net.path()[1], 20u);
}

TEST_F(NetTest, IsPlacedDefault) {
    Vertex& v1 = makeVertex(0, 0);
    Vertex& v2 = makeVertex(5, 5);
    Pin src(v1, 1.0);
    Pin tgt(v2, 1.0);

    Net net(src, tgt);
    EXPECT_FALSE(net.isPlaced());
}

TEST_F(NetTest, MarkPlaced) {
    Vertex& v1 = makeVertex(0, 0);
    Vertex& v2 = makeVertex(5, 5);
    Pin src(v1, 1.0);
    Pin tgt(v2, 1.0);

    Net net(src, tgt);
    net.markPlaced();
    EXPECT_TRUE(net.isPlaced());
}
