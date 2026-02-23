/*
 * Node.h
 *
 *  Created on: Mar 13, 2018
 *      Author: florian
 */

#ifndef NODE_H_
#define NODE_H_

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

    std::unordered_set<Node*> _adjacents;
    std::vector<Node*> _opposite;
    std::vector<std::size_t> _cover;
    int32_t _state = -1;
    int32_t _nodeId = 0;
    mutable int32_t _visited = 0;

    basic::Polygon_set_2Node _setPolygons;

    explicit Node(const int32_t nodeId) :
            _nodeId { nodeId } {

    }

    void setState(int32_t s) {
        _state = s;
    }

    bool operator <(const Node &n) const {

        if (_opposite.size() == n._opposite.size()) {
            return _adjacents.size() < n._adjacents.size();
        }
        return (_opposite.size() < n._opposite.size());

    }

    bool operator >(const Node &n) const {

        if (_opposite.size() == n._opposite.size()) {
            return _adjacents.size() > n._adjacents.size();
        }
        return (_opposite.size() > n._opposite.size());

    }

    void print(std::ostream& os) const {

        os << "id" << _nodeId << " s" << _state;
        os << " opp [";
        for (Node* nopp : _opposite) {
            os << nopp->_nodeId << " s" << nopp->_state << " ";
        }
        os << " ] ";
        os << " add [";
        for (Node* nopp : _adjacents) {
            os << nopp->_nodeId << " s" << nopp->_state << " ";
        }
        os << " ] ";
    }

    bool haveOppositeState() const {
        for (Node* n : _opposite) {
            if (n->_state != -1) {
                return true;
            }
        }
        return false;
    }
};
class StateSelect {
public:
    explicit StateSelect(const std::vector<Node*>& opposite) :
            _stateSelect(opposite.size(), false) {
        for (Node* opp : opposite) {
            add(opp->_state);
        }
    }

    void add(int32_t i) {
        if (-1 < i and i < static_cast<int32_t>(_stateSelect.size())) {
            _stateSelect.at(static_cast<std::size_t>(i)) = true;
        }
    }

    ;

    int32_t getNext() {
        for (; ite < _stateSelect.size(); ++ite) {
            if (!_stateSelect.at(ite)) {
                _stateSelect.at(ite) = true;
                return static_cast<int32_t>(ite);
            }
        }
        return static_cast<int32_t>(ite);
    }
    std::size_t ite = 0;
    std::vector<bool> _stateSelect;

};
struct NodeQueue {
public:

    explicit NodeQueue(Node& node) :
            _node(&node) {

    }
    Node& node() {
        return *_node;
    }
    Node& node() const {
        return *_node;
    }

    // use for priority queue , we inverse "<" to ">" because we want the smaller first
    bool operator <(const NodeQueue &n) const {
        return node() > n.node();
    }
private:
    Node* _node;
};

} /* namespace laby */

#endif /* NODE_H_ */
