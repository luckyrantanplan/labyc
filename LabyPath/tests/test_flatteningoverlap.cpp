/**
 * Unit tests for flatteningOverlap module: Intersection, Node, StateSelect,
 * NodeQueue, and Family classes.
 */

#include <gtest/gtest.h>

#include <queue>
#include <unordered_set>
#include <vector>

#include "flatteningOverlap/Family.h"
#include "flatteningOverlap/Node.h"

using namespace laby;

// ─── Intersection ────────────────────────────────────────────────────────────

TEST(IntersectionTest, OrdersIndicesAscending) {
    Intersection i(5, 3);
    EXPECT_EQ(i.first(), 3U);
    EXPECT_EQ(i.second(), 5U);
}

TEST(IntersectionTest, AlreadyOrdered) {
    Intersection i(2, 7);
    EXPECT_EQ(i.first(), 2U);
    EXPECT_EQ(i.second(), 7U);
}

TEST(IntersectionTest, SameIndex) {
    Intersection i(4, 4);
    EXPECT_EQ(i.first(), 4U);
    EXPECT_EQ(i.second(), 4U);
}

TEST(IntersectionTest, EqualityOperator) {
    Intersection a(3, 5);
    Intersection b(5, 3);
    EXPECT_EQ(a, b);
}

TEST(IntersectionTest, InequalityDifferentPairs) {
    Intersection a(1, 2);
    Intersection b(1, 3);
    EXPECT_FALSE(a == b);
}

TEST(IntersectionTest, HashConsistency) {
    std::hash<Intersection> hasher;
    Intersection a(10, 20);
    Intersection b(20, 10);
    EXPECT_EQ(hasher(a), hasher(b));
}

TEST(IntersectionTest, UnorderedSetLookup) {
    std::unordered_set<Intersection> s;
    s.emplace(3, 7);
    s.emplace(1, 5);

    EXPECT_EQ(s.count({7, 3}), 1U);
    EXPECT_EQ(s.count({5, 1}), 1U);
    EXPECT_EQ(s.count({2, 9}), 0U);
}

// ─── Node ────────────────────────────────────────────────────────────────────

TEST(NodeTest, ConstructorSetsId) {
    Node n(42);
    EXPECT_EQ(n.nodeId(), 42);
    EXPECT_EQ(n.state(), -1);
    EXPECT_EQ(n.visited(), 0);
}

TEST(NodeTest, SetState) {
    Node n(1);
    n.setState(3);
    EXPECT_EQ(n.state(), 3);
}

TEST(NodeTest, HaveOppositeStateReturnsFalseWhenAllDefault) {
    Node n(0);
    Node opp1(1);
    Node opp2(2);
    n.opposite().push_back(&opp1);
    n.opposite().push_back(&opp2);
    EXPECT_FALSE(n.haveOppositeState());
}

TEST(NodeTest, HaveOppositeStateReturnsTrueWhenOneSet) {
    Node n(0);
    Node opp1(1);
    Node opp2(2);
    opp2.setState(0);
    n.opposite().push_back(&opp1);
    n.opposite().push_back(&opp2);
    EXPECT_TRUE(n.haveOppositeState());
}

TEST(NodeTest, LessThanComparesByOppositesThenAdjacents) {
    Node a(1);
    Node b(2);
    Node dummy1(10);
    Node dummy2(11);
    Node dummy3(12);

    // a has 1 opposite, b has 2 opposites → a < b
    a.opposite().push_back(&dummy1);
    b.opposite().push_back(&dummy1);
    b.opposite().push_back(&dummy2);
    EXPECT_TRUE(a < b);
    EXPECT_FALSE(b < a);
}

TEST(NodeTest, LessThanTieBreaksOnAdjacents) {
    Node a(1);
    Node b(2);
    Node dummy1(10);
    Node dummy2(11);

    // Same opposites count (0), a has fewer adjacents
    a.adjacents().insert(&dummy1);
    b.adjacents().insert(&dummy1);
    b.adjacents().insert(&dummy2);
    EXPECT_TRUE(a < b);
}

TEST(NodeTest, PrintOutputContainsIdAndState) {
    Node n(5);
    n.setState(2);
    std::ostringstream oss;
    n.print(oss);
    std::string output = oss.str();
    EXPECT_NE(output.find("id5"), std::string::npos);
    EXPECT_NE(output.find("s2"), std::string::npos);
}

// ─── StateSelect ─────────────────────────────────────────────────────────────

TEST(StateSelectTest, GetNextReturnsFirstAvailable) {
    std::vector<Node*> emptyOpposite;
    StateSelect ss(emptyOpposite);
    // No states marked, so getNext returns 0 (but size is 0, returns 0)
    // With empty opposites, size is 0 → getNext returns 0
    EXPECT_EQ(ss.getNext(), 0);
}

TEST(StateSelectTest, SkipsOccupiedStates) {
    Node opp1(1);
    Node opp2(2);
    opp1.setState(0);
    opp2.setState(2);
    std::vector<Node*> opposites = {&opp1, &opp2};
    StateSelect ss(opposites);

    // States 0 and 2 are taken, getNext should return 1
    int32_t next = ss.getNext();
    EXPECT_EQ(next, 1);
}

TEST(StateSelectTest, MarkOccupiedInvalidIndexIgnored) {
    Node opp1(1);
    opp1.setState(-1);
    std::vector<Node*> opposites = {&opp1};
    StateSelect ss(opposites);

    // -1 is invalid, so state 0 should be available
    EXPECT_EQ(ss.getNext(), 0);
}

TEST(StateSelectTest, SequentialGetNextExhaustsSlots) {
    Node opp1(1);
    Node opp2(2);
    Node opp3(3);
    opp1.setState(-1);
    opp2.setState(-1);
    opp3.setState(-1);
    std::vector<Node*> opposites = {&opp1, &opp2, &opp3};
    StateSelect ss(opposites);

    // All -1 → no valid states, so all slots are free
    EXPECT_EQ(ss.getNext(), 0);
    EXPECT_EQ(ss.getNext(), 1);
    EXPECT_EQ(ss.getNext(), 2);
    // Exhausted → returns size
    EXPECT_EQ(ss.getNext(), 3);
}

// ─── NodeQueue ───────────────────────────────────────────────────────────────

TEST(NodeQueueTest, WrapsNode) {
    Node n(99);
    n.setState(5);
    NodeQueue nq(n);
    EXPECT_EQ(nq.node().nodeId(), 99);
    EXPECT_EQ(nq.node().state(), 5);
}

TEST(NodeQueueTest, PriorityQueueOrdersSmallerFirst) {
    // NodeQueue uses inverted < so priority_queue gives smallest first
    Node a(1);
    Node b(2);
    Node c(3);
    Node dummy1(10);
    Node dummy2(11);
    Node dummy3(12);

    // a: 0 opposites, b: 1 opposite, c: 2 opposites
    b.opposite().push_back(&dummy1);
    c.opposite().push_back(&dummy1);
    c.opposite().push_back(&dummy2);

    std::priority_queue<NodeQueue> pq;
    pq.emplace(c);
    pq.emplace(a);
    pq.emplace(b);

    // Top should be 'a' (fewest opposites)
    EXPECT_EQ(pq.top().node().nodeId(), 1);
    pq.pop();
    EXPECT_EQ(pq.top().node().nodeId(), 2);
    pq.pop();
    EXPECT_EQ(pq.top().node().nodeId(), 3);
}

// ─── Family ──────────────────────────────────────────────────────────────────

TEST(FamilyTest, DefaultConstructionEmpty) {
    Family f;
    EXPECT_TRUE(f.intersections().empty());
    EXPECT_TRUE(f.patches().empty());
}
