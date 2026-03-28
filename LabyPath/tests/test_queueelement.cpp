#include "Anisotrop/QueueElement.h"
#include "GeomData.h"

#include <gtest/gtest.h>

using namespace laby;
using namespace laby::aniso;

namespace {

constexpr double kCoordZero = 0.0;
constexpr double kCoordOne = 1.0;
constexpr double kCoordTwo = 2.0;

constexpr int32_t kDirectionUnset = -1;
constexpr int32_t kDirectionThree = 3;
constexpr int32_t kParentUnset = -1;
constexpr int32_t kParentFive = 5;
constexpr int32_t kHigherDistance = 10;
constexpr int32_t kLowerDistance = 5;

class QueueElementFixture : public ::testing::Test {
  protected:
    auto makeVertex(double xCoord, double yCoord) -> Vertex& {
        auto vertexHandle = _arrangement.insert_in_face_interior(Point_2(xCoord, yCoord),
                                                                 _arrangement.unbounded_face());
        return *vertexHandle;
    }

  private:
    Arrangement_2 _arrangement;
};

} // namespace

TEST_F(QueueElementFixture, Construction) {
    Vertex& vertex = makeVertex(kCoordOne, kCoordTwo);
    QueueElement queueElement(vertex);

    EXPECT_EQ(queueElement.direction(), kDirectionUnset);
    EXPECT_EQ(queueElement.parent(), kParentUnset);
    EXPECT_DOUBLE_EQ(CGAL::to_double(queueElement.vertex().point().x()), kCoordOne);
}

TEST_F(QueueElementFixture, ClearResetsState) {
    Vertex& vertex = makeVertex(kCoordZero, kCoordZero);
    QueueElement queueElement(vertex);
    queueElement.setDirection(kDirectionThree);
    queueElement.setParent(kParentFive);
    queueElement.cost() = QueueCost();

    queueElement.clear();
    EXPECT_EQ(queueElement.direction(), kDirectionUnset);
    EXPECT_EQ(queueElement.parent(), kParentUnset);
}

TEST_F(QueueElementFixture, NotInQueueByDefault) {
    Vertex& vertex = makeVertex(kCoordZero, kCoordZero);
    QueueElement queueElement(vertex);
    EXPECT_FALSE(queueElement.isInQueue());
}

TEST_F(QueueElementFixture, PushInAndIsInQueue) {
    Vertex& vertex = makeVertex(kCoordZero, kCoordZero);
    QueueElement queueElement(vertex);
    PriorityQueue priorityQueue;

    queueElement.pushIn(priorityQueue);
    EXPECT_TRUE(queueElement.isInQueue());
}

TEST_F(QueueElementFixture, ResetHandleClearsQueueState) {
    Vertex& vertex = makeVertex(kCoordZero, kCoordZero);
    QueueElement queueElement(vertex);
    PriorityQueue priorityQueue;

    queueElement.pushIn(priorityQueue);
    EXPECT_TRUE(queueElement.isInQueue());

    queueElement.resetHandle();
    EXPECT_FALSE(queueElement.isInQueue());
}

TEST_F(QueueElementFixture, PriorityOrdering) {
    Vertex& firstVertex = makeVertex(kCoordZero, kCoordZero);
    Vertex& secondVertex = makeVertex(kCoordOne, kCoordOne);
    QueueElement firstQueueElement(firstVertex);
    QueueElement secondQueueElement(secondVertex);

    firstQueueElement.cost().distance() = kHigherDistance;
    secondQueueElement.cost().distance() = kLowerDistance;

    PriorityQueue priorityQueue;
    firstQueueElement.pushIn(priorityQueue);
    secondQueueElement.pushIn(priorityQueue);

    EXPECT_EQ(priorityQueue.top(), &secondQueueElement);
}

TEST_F(QueueElementFixture, PolyConvexDefaultEmpty) {
    Vertex& vertex = makeVertex(kCoordZero, kCoordZero);
    QueueElement queueElement(vertex);
    EXPECT_TRUE(queueElement.polyConvex().empty());
}
