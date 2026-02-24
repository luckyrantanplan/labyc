#include <gtest/gtest.h>
#include "GeomData.h"
#include "Anisotrop/QueueElement.h"

using namespace laby;
using namespace laby::aniso;

class QueueElementTest : public ::testing::Test {
protected:
    Arrangement_2 arr;

    Vertex& makeVertex(double x, double y) {
        auto vh = arr.insert_in_face_interior(Point_2(x, y), arr.unbounded_face());
        return *vh;
    }
};

TEST_F(QueueElementTest, Construction) {
    Vertex& v = makeVertex(1.0, 2.0);
    QueueElement qe(v);

    EXPECT_EQ(qe.direction, -1);
    EXPECT_EQ(qe.parent, -1);
    EXPECT_DOUBLE_EQ(CGAL::to_double(qe._vertex.point().x()), 1.0);
}

TEST_F(QueueElementTest, ClearResetsState) {
    Vertex& v = makeVertex(0, 0);
    QueueElement qe(v);
    qe.direction = 3;
    qe.parent = 5;
    qe.cost = QueueCost();

    qe.clear();
    EXPECT_EQ(qe.direction, -1);
    EXPECT_EQ(qe.parent, -1);
}

TEST_F(QueueElementTest, NotInQueueByDefault) {
    Vertex& v = makeVertex(0, 0);
    QueueElement qe(v);
    EXPECT_FALSE(qe.isInQueue());
}

TEST_F(QueueElementTest, PushInAndIsInQueue) {
    Vertex& v = makeVertex(0, 0);
    QueueElement qe(v);
    PriorityQueue pq;

    qe.pushIn(pq);
    EXPECT_TRUE(qe.isInQueue());
}

TEST_F(QueueElementTest, ResetHandleClearsQueueState) {
    Vertex& v = makeVertex(0, 0);
    QueueElement qe(v);
    PriorityQueue pq;

    qe.pushIn(pq);
    EXPECT_TRUE(qe.isInQueue());

    qe.resetHandle();
    EXPECT_FALSE(qe.isInQueue());
}

TEST_F(QueueElementTest, PriorityOrdering) {
    Vertex& v1 = makeVertex(0, 0);
    Vertex& v2 = makeVertex(1, 1);
    QueueElement qe1(v1);
    QueueElement qe2(v2);

    // Lower cost should come first (min-heap)
    qe1.cost.distance = 10;
    qe2.cost.distance = 5;

    PriorityQueue pq;
    qe1.pushIn(pq);
    qe2.pushIn(pq);

    // Top should be the lower cost element
    EXPECT_EQ(pq.top(), &qe2);
}

TEST_F(QueueElementTest, PolyConvexDefaultEmpty) {
    Vertex& v = makeVertex(0, 0);
    QueueElement qe(v);
    EXPECT_TRUE(qe._pc.empty());
}
