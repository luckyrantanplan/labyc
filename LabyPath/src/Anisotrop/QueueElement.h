/*
 * QueueElement.h
 *
 *  Created on: Mar 27, 2018
 *      Author: florian
 */

#ifndef ANISOTROP_QUEUEELEMENT_H_
#define ANISOTROP_QUEUEELEMENT_H_

#include <bits/stdint-intn.h>
#include <boost/heap/detail/stable_heap.hpp>
#include <boost/heap/pairing_heap.hpp>
#include <boost/heap/policies.hpp>

#include "../GeomData.h"
#include "../PolyConvex.h"
#include "QueueCost.h"

namespace laby {
namespace aniso {

class QueueElement {

    class Element_greater {

    public:

        bool operator()(const QueueElement* lhs, const QueueElement* rhs) const {

            return lhs->cost > rhs->cost;

        }
    };
public:

    explicit QueueElement(Vertex& ver) :
            _vertex { ver } {

    }

    typedef boost::heap::pairing_heap<QueueElement*, boost::heap::compare<Element_greater>> PriorityQueue;
    typedef PriorityQueue::handle_type HandleType;

    void resetHandle() {
        handle = HandleType { };
    }

    void clear() {
        resetHandle();
        cost = QueueCost();
        direction = -1;
        parent = -1;
        _pc.clear();
    }

    void pushIn(PriorityQueue& pq) {
        handle = pq.push(this);
    }

    bool isInQueue() {
        return handle != HandleType { };
    }

    Vertex& _vertex;
    PolyConvex _pc;
    QueueCost cost;
    int32_t direction = -1;
    int32_t parent = -1;
    HandleType handle;            //Index in MMMPriortyQueue

};

typedef QueueElement::PriorityQueue PriorityQueue;
} /* namespace aniso */
} /* namespace laby */

#endif /* ANISOTROP_QUEUEELEMENT_H_ */
