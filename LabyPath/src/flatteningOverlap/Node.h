/**
 * @file Node.h
 * @brief Conflict graph nodes for the overlap resolution algorithm.
 *
 * ## Data Structures
 *
 * ### Node
 * Represents a connected component of overlapping polygons in the conflict
 * graph.  Each Node covers a set of PolyConvex indices and is connected to:
 *
 * - **_opposite**: Nodes that share the same overlap region but belong to a
 *   different connected component (patch).  Opposite nodes MUST receive
 *   different rendering states (graph coloring constraint).
 *
 * - **_adjacents**: Nodes from nearby (but non-overlapping) families.
 *   Adjacent nodes PREFER alternating states (0↔1) for visual clarity,
 *   but this is a soft constraint.
 *
 * - **_cover**: Indices into the PolyConvex vector – the polygon IDs that
 *   this node "covers" (i.e., is responsible for rendering).
 *
 * - **_state**: The rendering state assigned by graph coloring (-1 = unassigned).
 *   States are non-negative integers; opposite nodes get different states.
 *
 * - **_visited**: Mutable traversal flag used during BFS/DFS in
 *   PathRendering::nodeAdjacence() and NodeRendering::render().
 *   Declared `mutable` because traversal reads modify it on const objects.
 *   Value 0 = unvisited, 1 = visited, -1 = marked during adjacency BFS.
 *
 * - **_setPolygons**: CGAL polygon set storing the geometric union of the
 *   covered polygons, used for arrangement overlay in NodeRendering.
 *
 * ### StateSelect
 * Greedy state allocator: given a list of opposite nodes, tracks which
 * states are already occupied and returns the next available state.
 *
 * ### NodeQueue
 * Priority-queue wrapper that orders Nodes by ascending degree (fewest
 * opposites first), ensuring the greedy coloring processes simpler
 * conflicts before complex ones.
 */

#ifndef NODE_H_
#define NODE_H_

#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../basic/AugmentedPolygonSet.h"

namespace laby {

class Node {
public:
    /// Nodes from nearby non-overlapping families (soft alternating-state constraint).
    std::unordered_set<Node*> _adjacents; // NOLINT(misc-non-private-member-variables-in-classes)

    /// Nodes sharing the same overlap region but from a different patch
    /// (hard different-state constraint for graph coloring).
    std::vector<Node*> _opposite; // NOLINT(misc-non-private-member-variables-in-classes)

    /// Indices into the PolyConvex vector that this node is responsible for.
    std::vector<std::size_t> _cover; // NOLINT(misc-non-private-member-variables-in-classes)

    /// Rendering state assigned by graph coloring.  -1 = unassigned.
    int32_t _state = -1; // NOLINT(misc-non-private-member-variables-in-classes)

    /// Unique identifier for debugging/logging.
    int32_t _nodeId = 0; // NOLINT(misc-non-private-member-variables-in-classes)

    /// Traversal flag: 0 = unvisited, 1 = visited, -1 = BFS-marked.
    /// Declared mutable because BFS/DFS traversals modify it on const objects.
    mutable int32_t _visited = 0; // NOLINT(misc-non-private-member-variables-in-classes)

    /// Geometric union of covered polygons (used for arrangement overlay).
    basic::Polygon_set_2Node _setPolygons; // NOLINT(misc-non-private-member-variables-in-classes)

    explicit Node(const int32_t nodeId) : _nodeId{nodeId} {}

    void setState(int32_t s) { _state = s; }

    bool operator<(const Node& n) const {
        if (_opposite.size() == n._opposite.size()) {
            return _adjacents.size() < n._adjacents.size();
        }
        return (_opposite.size() < n._opposite.size());
    }

    bool operator>(const Node& n) const {
        if (_opposite.size() == n._opposite.size()) {
            return _adjacents.size() > n._adjacents.size();
        }
        return (_opposite.size() > n._opposite.size());
    }

    void print(std::ostream& os) const {
        os << "id" << _nodeId << " s" << _state;
        os << " opp [";
        for (Node* oppositeNode : _opposite) {
            os << oppositeNode->_nodeId << " s" << oppositeNode->_state << " ";
        }
        os << " ] ";
        os << " adj [";
        for (Node* adjacentNode : _adjacents) {
            os << adjacentNode->_nodeId << " s" << adjacentNode->_state << " ";
        }
        os << " ] ";
    }

    /// Returns true if any opposite node has been assigned a state (!= -1).
    bool haveOppositeState() const {
        return std::any_of(_opposite.begin(), _opposite.end(), [](const Node* n) { return n->_state != -1; });
    }
};

/**
 * Greedy state allocator for graph coloring.
 *
 * Given a list of opposite nodes, tracks which state indices are already
 * occupied and returns the next available one via getNext().
 */
class StateSelect {
public:
    explicit StateSelect(const std::vector<Node*>& opposite) : _occupied_states(opposite.size(), false) {
        for (Node* opp : opposite) {
            markOccupied(opp->_state);
        }
    }

    void markOccupied(int32_t state_index) {
        if (state_index >= 0 && state_index < static_cast<int32_t>(_occupied_states.size())) {
            _occupied_states.at(static_cast<std::size_t>(state_index)) = true;
        }
    }

    int32_t getNext() {
        for (; _current_index < _occupied_states.size(); ++_current_index) {
            if (!_occupied_states.at(_current_index)) {
                _occupied_states.at(_current_index) = true;
                return static_cast<int32_t>(_current_index);
            }
        }
        return static_cast<int32_t>(_current_index);
    }

    [[nodiscard]] std::size_t currentIndex() const { return _current_index; }
    [[nodiscard]] const std::vector<bool>& occupiedStates() const { return _occupied_states; }

private:
    std::size_t _current_index = 0;
    std::vector<bool> _occupied_states;
};

/**
 * Priority-queue wrapper: orders Nodes by ascending degree (fewest opposites
 * first) so the greedy coloring handles simpler conflicts before complex ones.
 */
struct NodeQueue {
public:
    explicit NodeQueue(Node& node) : _node(&node) {}
    [[nodiscard]] Node& node() { return *_node; }
    [[nodiscard]] Node& node() const { return *_node; }

    // Inverted comparison: priority_queue gives max by default,
    // so inverting '<' to '>' makes it return the smallest first.
    bool operator<(const NodeQueue& n) const { return node() > n.node(); }

private:
    Node* _node;
};

} /* namespace laby */

#endif /* NODE_H_ */
