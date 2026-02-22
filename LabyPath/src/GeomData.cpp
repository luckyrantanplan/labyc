/*
 * GeomData.cpp
 *
 *  Created on: Mar 20, 2018
 *      Author: florian
 */

#include "GeomData.h"

#include <boost/geometry/geometries/point_xy.hpp>
#include <CGAL/Arr_dcel_base.h>
#include <CGAL/Arr_geometry_traits/Curve_data_aux.h>
#include <CGAL/number_utils.h>
#include <CGAL/Point_2.h>

#include "basic/SimplifyLines.h"

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
