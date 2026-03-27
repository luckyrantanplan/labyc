/*
 * QueueElement.h
 *
 *  Created on: Mar 27, 2018
 *      Author: florian
 */

#ifndef ANISOTROP_QUEUEELEMENT_H_
#define ANISOTROP_QUEUEELEMENT_H_

#include <boost/heap/detail/stable_heap.hpp>
#include <boost/heap/pairing_heap.hpp>
#include <boost/heap/policies.hpp>
#include <cstdint>

#include "../GeomData.h"
#include "../PolyConvex.h"
#include "QueueCost.h"

namespace laby::aniso {

class QueueElement {

    class ElementGreater {

      public:
        auto operator()(const QueueElement* lhs, const QueueElement* rhs) const -> bool {
            return lhs->cost() > rhs->cost();
        }
    };

  public:
    explicit QueueElement(Vertex& vertex) : _vertex(&vertex) {}

    using PriorityQueue =
        boost::heap::pairing_heap<QueueElement*, boost::heap::compare<ElementGreater>>;
    using HandleType = PriorityQueue::handle_type;

    auto vertex() -> Vertex& {
        return *_vertex;
    }

    [[nodiscard]] auto vertex() const -> const Vertex& {
        return *_vertex;
    }

    auto polyConvex() -> PolyConvex& {
        return _polyConvex;
    }

    [[nodiscard]] auto polyConvex() const -> const PolyConvex& {
        return _polyConvex;
    }

    auto cost() -> QueueCost& {
        return _cost;
    }

    [[nodiscard]] auto cost() const -> const QueueCost& {
        return _cost;
    }

    void setDirection(int32_t direction) {
        _direction = direction;
    }

    [[nodiscard]] auto direction() const -> int32_t {
        return _direction;
    }

    void setParent(int32_t parent) {
        _parent = parent;
    }

    [[nodiscard]] auto parent() const -> int32_t {
        return _parent;
    }

    auto handle() -> HandleType& {
        return _handle;
    }

    [[nodiscard]] auto handle() const -> const HandleType& {
        return _handle;
    }

    void resetHandle() {
        _handle = HandleType{};
    }

    void clear() {
        resetHandle();
        _cost = QueueCost();
        _direction = -1;
        _parent = -1;
        _polyConvex.clear();
    }

    void pushIn(PriorityQueue& queue) {
        _handle = queue.push(this);
    }

    [[nodiscard]] auto isInQueue() const -> bool {
        return _handle != HandleType{};
    }

  private:
    Vertex* _vertex = nullptr;
    PolyConvex _polyConvex;
    QueueCost _cost;
    int32_t _direction = -1;
    int32_t _parent = -1;
    HandleType _handle; // Index in MMMPriortyQueue
};

using PriorityQueue = QueueElement::PriorityQueue;
} // namespace laby::aniso

#endif /* ANISOTROP_QUEUEELEMENT_H_ */
