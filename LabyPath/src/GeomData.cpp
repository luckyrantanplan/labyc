/*
 * GeomData.cpp
 *
 *  Created on: Mar 20, 2018
 *      Author: florian
 */

#include "GeomData.h"

#include <cstdint>

#include "basic/RangeHelper.h"

namespace laby {

auto EdgeInfo::getNextHalfedge(int32_t visited, const Vertex& vertex) const
    -> const Halfedge* {

    for (const Halfedge& halfedge : RangeHelper::make(vertex.incident_halfedges())) {
        const laby::EdgeInfo& data = halfedge.curve().data();
        if (data == *this && data.getVisit() != visited) {
            halfedge.curve().data().setVisit(visited);
            return &halfedge;
        }
    }
    return nullptr;
}

} /* namespace laby */
