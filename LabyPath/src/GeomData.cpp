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

const Halfedge* EdgeInfo::getNextHalfedge(int32_t visited, const Vertex& v) const {

    for (const Halfedge& he : RangeHelper::make(v.incident_halfedges())) {
        const laby::EdgeInfo& data = he.curve().data();
        if (data == *this && data.getVisit() != visited) {
            he.curve().data().setVisit(visited);
            return &he;
        }
    }
    return nullptr;
}

} /* namespace laby */
