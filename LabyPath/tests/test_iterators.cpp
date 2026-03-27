/**
 * @file test_iterators.cpp
 * @brief Unit tests for iterator/circulator patterns: NumericRange const-correctness,
 *        RangeHelper with mock circulators, and graph pointer-through-queue safety.
 */

#include <gtest/gtest.h>
#include "basic/NumericRange.h"
#include "basic/RangeHelper.h"
#include "flatteningOverlap/Node.h"

#include <cstddef>
#include <cstdint>
#include <queue>
#include <vector>

namespace laby {
namespace {

// ============================================================
// NumericRange: const-correctness and float precision
// ============================================================

TEST(NumericRangeConstTest, ConstRangeIteration) {
    const NumericRange<int32_t> range(0, 6, 2);
    std::vector<int32_t> values;
    for (const int32_t v : range) {
        values.push_back(v);
    }
    // 0, 2, 4, 6
    ASSERT_EQ(values.size(), 4U);
    EXPECT_EQ(values.front(), 0);
    EXPECT_EQ(values.back(), 6);
}

TEST(NumericRangeConstTest, ConstDoubleRange) {
    const NumericRange<double> range(0.0, 1.0, 0.5);
    std::vector<double> values;
    for (const double v : range) {
        values.push_back(v);
    }
    // 0.0, 0.5, 1.0
    ASSERT_EQ(values.size(), 3U);
    EXPECT_NEAR(values[0], 0.0, 1e-12);
    EXPECT_NEAR(values[1], 0.5, 1e-12);
    EXPECT_NEAR(values[2], 1.0, 1e-12);
}

TEST(NumericRangeConstTest, ConstGetValue) {
    const NumericRange<int32_t> range(10, 20, 5);
    EXPECT_EQ(range.getValue(0), 10);
    EXPECT_EQ(range.getValue(1), 15);
    EXPECT_EQ(range.getValue(2), 20);
}

TEST(NumericRangeFloatPrecisionTest, FloatStepNoTruncation) {
    // Regression: (4.0 - 0.0) / 0.5 = 8.0 which could truncate to 7
    // if static_cast<int32_t> is used instead of std::lround.
    const NumericRange<double> range(0.0, 4.0, 0.5);
    std::vector<double> values;
    for (const double v : range) {
        values.push_back(v);
    }
    // 0.0, 0.5, 1.0, ..., 4.0 = 9 values
    ASSERT_EQ(values.size(), 9U);
    EXPECT_NEAR(values.back(), 4.0, 1e-12);
}

TEST(NumericRangeFloatPrecisionTest, ThirdStepNoTruncation) {
    // (1.0 - 0.0) / (1.0/3.0) ≈ 3.0 - floating point may give 2.9999...
    const NumericRange<double> range(0.0, 1.0, 1.0 / 3.0);
    std::vector<double> values;
    for (const double v : range) {
        values.push_back(v);
    }
    // 0.0, 0.333..., 0.666..., 1.0 = 4 values
    ASSERT_EQ(values.size(), 4U);
    EXPECT_NEAR(values[0], 0.0, 1e-12);
    EXPECT_NEAR(values[3], 1.0, 1e-12);
}

TEST(NumericRangeIteratorTest, ComparisonIsConst) {
    const NumericRange<int32_t> range(0, 2, 1);
    auto it1 = range.begin();
    auto it2 = range.end();
    // These should compile on const iterators
    EXPECT_TRUE(it1 != it2);
    EXPECT_FALSE(it1 == it2);
}

TEST(NumericRangeIteratorTest, DereferenceIsConst) {
    const NumericRange<int32_t> range(5, 5, 1);
    auto it = range.begin();
    // operator* should be callable on const-qualified iterator
    const int32_t val = *it;
    EXPECT_EQ(val, 5);
}

// ============================================================
// RangeHelper: Mock circulator test
// ============================================================

/// A minimal mock circulator for testing HalfedgeRange without CGAL.
struct MockCirculator {
    using reference = int32_t&;

    int32_t* data;    
    std::size_t size; 
    std::size_t pos;  

    MockCirculator(int32_t* d, std::size_t s, std::size_t p = 0) 
        : data(d), size(s), pos(p) {}

    reference operator*() const { return data[pos]; }
    MockCirculator& operator++() {
        pos = (pos + 1) % size;
        return *this;
    }
    bool operator==(const MockCirculator& o) const { return pos == o.pos; }
    bool operator!=(const MockCirculator& o) const { return pos != o.pos; }
};

TEST(RangeHelperTest, HalfedgeRangeIteratesAll) {
    int32_t data[] = {10, 20, 30};
    const MockCirculator circ(data, 3, 0);
    HalfedgeRange<MockCirculator> range(circ);

    std::vector<int32_t> values;
    for (auto& v : range) {
        values.push_back(v);
    }
    ASSERT_EQ(values.size(), 3U);
    EXPECT_EQ(values[0], 10);
    EXPECT_EQ(values[1], 20);
    EXPECT_EQ(values[2], 30);
}

TEST(RangeHelperTest, HalfedgeRangeSingleElement) {
    int32_t data[] = {42};
    const MockCirculator circ(data, 1, 0);
    HalfedgeRange<MockCirculator> range(circ);

    std::vector<int32_t> values;
    for (auto& v : range) {
        values.push_back(v);
    }
    ASSERT_EQ(values.size(), 1U);
    EXPECT_EQ(values[0], 42);
}

TEST(RangeHelperTest, RangeIteratorBasic) {
    std::vector<int32_t> data = {1, 2, 3, 4, 5};
    RangeIterator<std::vector<int32_t>::iterator> range(data.begin(), data.end());

    std::vector<int32_t> values;
    for (const int32_t v : range) {
        values.push_back(v);
    }
    ASSERT_EQ(values.size(), 5U);
    EXPECT_EQ(values[0], 1);
    EXPECT_EQ(values[4], 5);
}

TEST(RangeHelperTest, RangeIteratorEmpty) {
    std::vector<int32_t> data;
    RangeIterator<std::vector<int32_t>::iterator> range(data.begin(), data.end());

    int32_t count = 0;
    for ([[maybe_unused]] const int32_t v : range) {
        ++count;
    }
    EXPECT_EQ(count, 0);
}

TEST(RangeHelperTest, MakeWithIteratorPair) {
    std::vector<int32_t> data = {10, 20, 30};
    auto range = RangeHelper::make(data.begin(), data.end());

    std::vector<int32_t> values;
    for (const int32_t v : range) {
        values.push_back(v);
    }
    ASSERT_EQ(values.size(), 3U);
    EXPECT_EQ(values[0], 10);
}

// ============================================================
// Graph: NodeQueue pointer-through-queue safety
// ============================================================

TEST(NodeQueueTest, PopDoesNotInvalidateNodeReference) {
    // Verify that popping from priority_queue<NodeQueue> does not
    // invalidate the Node& reference obtained before pop.
    std::vector<Node> nodes;
    nodes.reserve(3);
    nodes.emplace_back(0);
    nodes.emplace_back(1);
    nodes.emplace_back(2);

    // Give different opposite counts for priority ordering
    nodes[0]._opposite.push_back(&nodes[1]);
    nodes[0]._opposite.push_back(&nodes[2]);    // degree 2
    nodes[1]._opposite.push_back(nodes.data()); // degree 1
    nodes[2]._opposite.push_back(nodes.data()); // degree 1

    std::priority_queue<NodeQueue> queue;
    queue.emplace(nodes[0]);
    queue.emplace(nodes[1]);
    queue.emplace(nodes[2]);

    // Get reference before pop
    Node& topNode = queue.top().node();
    const int32_t topId = topNode._nodeId;
    queue.pop();

    // Node reference should still be valid (points into nodes vector)
    EXPECT_EQ(topNode._nodeId, topId);
    topNode._state = 99;
    EXPECT_EQ(topNode._state, 99);
}

TEST(NodeQueueTest, MinDegreeFirst) {
    std::vector<Node> nodes;
    nodes.reserve(3);
    nodes.emplace_back(0);
    nodes.emplace_back(1);
    nodes.emplace_back(2);

    nodes[0]._opposite = {&nodes[1], &nodes[2]};
    nodes[1]._opposite = {};
    nodes[2]._opposite = {nodes.data()};

    std::priority_queue<NodeQueue> queue;
    queue.emplace(nodes[0]);
    queue.emplace(nodes[1]);
    queue.emplace(nodes[2]);

    // Should come out in ascending degree order: 0, 1, 2
    EXPECT_EQ(queue.top().node()._nodeId, 1); // degree 0
    queue.pop();
    EXPECT_EQ(queue.top().node()._nodeId, 2); // degree 1
    queue.pop();
    EXPECT_EQ(queue.top().node()._nodeId, 0); // degree 2
    queue.pop();
}

// ============================================================
// Node: haveOppositeState with std::any_of
// ============================================================

TEST(NodeAnyOfTest, NoOpposites) {
    const Node node(0);
    EXPECT_FALSE(node.haveOppositeState());
}

TEST(NodeAnyOfTest, AllUnassigned) {
    Node n0(0);
    Node n1(1);
    Node n2(2);
    n0._opposite = {&n1, &n2};
    EXPECT_FALSE(n0.haveOppositeState());
}

TEST(NodeAnyOfTest, OneAssigned) {
    Node n0(0);
    Node n1(1);
    Node n2(2);
    n1._state = 0;
    n0._opposite = {&n1, &n2};
    EXPECT_TRUE(n0.haveOppositeState());
}

TEST(NodeAnyOfTest, AllAssigned) {
    Node n0(0);
    Node n1(1);
    Node n2(2);
    n1._state = 0;
    n2._state = 1;
    n0._opposite = {&n1, &n2};
    EXPECT_TRUE(n0.haveOppositeState());
}

} // namespace
} // namespace laby
