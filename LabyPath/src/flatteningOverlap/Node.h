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
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../basic/AugmentedPolygonSet.h"

namespace laby {

class Node {
  public:
    /// Nodes from nearby non-overlapping families (soft alternating-state constraint).
    std::unordered_set<Node*> _adjacents;

    /// Nodes sharing the same overlap region but from a different patch
    /// (hard different-state constraint for graph coloring).
    std::vector<Node*> _opposite;

    /// Indices into the PolyConvex vector that this node is responsible for.
    std::vector<std::size_t> _cover;

    /// Rendering state assigned by graph coloring.  -1 = unassigned.
    int32_t _state = -1;

    /// Unique identifier for debugging/logging.
    int32_t _nodeId = 0;

    /// Traversal flag: 0 = unvisited, 1 = visited, -1 = BFS-marked.
    /// Declared mutable because BFS/DFS traversals modify it on const objects.
    mutable int32_t _visited = 0;

    /// Geometric union of covered polygons (used for arrangement overlay).
    basic::Polygon_set_2Node _setPolygons;

    explicit Node(const int32_t nodeId) : _nodeId{nodeId} {}

    void setState(int32_t state) {
        _state = state;
    }

    auto operator<(const Node& node) const -> bool {
        if (_opposite.size() == node._opposite.size()) {
            return _adjacents.size() < node._adjacents.size();
        }
        return (_opposite.size() < node._opposite.size());
    }

    auto operator>(const Node& node) const -> bool {
        if (_opposite.size() == node._opposite.size()) {
            return _adjacents.size() > node._adjacents.size();
        }
        return (_opposite.size() > node._opposite.size());
    }

    void print(std::ostream& outputStream) const {
        outputStream << "id" << _nodeId << " s" << _state;
        outputStream << " opp [";
        for (Node* oppositeNode : _opposite) {
            outputStream << oppositeNode->_nodeId << " s" << oppositeNode->_state << " ";
        }
        outputStream << " ] ";
        outputStream << " adj [";
        for (Node* adjacentNode : _adjacents) {
            outputStream << adjacentNode->_nodeId << " s" << adjacentNode->_state << " ";
        }
        outputStream << " ] ";
    }

    /// Returns true if any opposite node has been assigned a state (!= -1).
    auto haveOppositeState() const -> bool {
        return std::any_of(_opposite.begin(), _opposite.end(),
                           [](const Node* n) { return n->_state != -1; });
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
    explicit StateSelect(const std::vector<Node*>& opposite)
        : _occupied_states(opposite.size(), false) {
        for (Node* opp : opposite) {
            markOccupied(opp->_state);
        }
    }

    void markOccupied(int32_t state_index) {
        if (state_index >= 0 && state_index < static_cast<int32_t>(_occupied_states.size())) {
            _occupied_states.at(static_cast<std::size_t>(state_index)) = true;
        }
    }

    auto getNext() -> int32_t {
        for (; _current_index < _occupied_states.size(); ++_current_index) {
            if (!_occupied_states.at(_current_index)) {
                _occupied_states.at(_current_index) = true;
                return static_cast<int32_t>(_current_index);
            }
        }
        return static_cast<int32_t>(_current_index);
    }

    [[nodiscard]] auto currentIndex() const -> std::size_t {
        return _current_index;
    }
    [[nodiscard]] auto occupiedStates() const -> const std::vector<bool>& {
        return _occupied_states;
    }

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
    [[nodiscard]] auto node() const -> Node& {
        return *_node;
    }

    // Inverted comparison: priority_queue gives max by default,
    // so inverting '<' to '>' makes it return the smallest first.
    auto operator<(const NodeQueue& nodeQueue) const -> bool {
        return node() > nodeQueue.node();
    }

  private:
    Node* _node;
};

} /* namespace laby */

#endif /* NODE_H_ */
