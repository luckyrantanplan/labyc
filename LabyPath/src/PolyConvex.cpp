/*
 * PolyConvex.cpp
 *
 *  Created on: Mar 15, 2018
 *      Author: florian
 */

#include "PolyConvex.h"

#include <CGAL/Arrangement_on_surface_2.h>
#include <CGAL/Segment_2.h>
#include <CGAL/intersection_2.h>
#include "basic/EasyProfilerCompat.h"

#include "basic/LinearGradient.h"

namespace laby {

bool PolyConvex::testConvexPolyIntersect(const Linear_polygon & a, const Linear_polygon& b) {
    EASY_FUNCTION();

    for (const CGAL::Segment_2<Kernel>& ea : RangeHelper::make(a.edges_begin(), a.edges_end())) {
        for (const CGAL::Segment_2<Kernel>& eb : RangeHelper::make(b.edges_begin(), b.edges_end())) {
            if (CGAL::do_intersect(ea, eb)) {
                return true;
            }
        }
    }

    if (a.has_on_bounded_side(b.vertex(0))) {
        return true;
    }
    if (b.has_on_bounded_side(a.vertex(0))) {
        return true;
    }
    return false;

}

void PolyConvex::connect(std::size_t first, std::size_t second, std::vector<PolyConvex>& polyConvexList, const Point_2& center) {
// we push in the vector, before to take reference on it (otherwise we get memory corruption)
    polyConvexList.emplace_back();

    PolyConvex& pc1 = polyConvexList.at(first);
    PolyConvex& pc2 = polyConvexList.at(second);

    pc1._adjacents.push_back(pc2._id);
    pc2._adjacents.push_back(pc1._id);

    PolyConvex& triangle = polyConvexList.back();
    triangle._id = polyConvexList.size() - 1;
    triangle._geometry = PolygonTools::createJoinTriangle(pc1._originalTrapeze, pc2._originalTrapeze, center);

    triangle._adjacents.push_back(pc2._id);
    triangle._adjacents.push_back(pc1._id);
    pc1._adjacents.push_back(triangle._id);
    pc2._adjacents.push_back(triangle._id);

}

void PolyConvex::connect(std::size_t first, std::size_t second, std::vector<PolyConvex>& polyConvexList) {
    PolyConvex& pc1 = polyConvexList.at(first);
    PolyConvex& pc2 = polyConvexList.at(second);

    pc1._adjacents.push_back(pc2._id);
    pc2._adjacents.push_back(pc1._id);

}

void PolyConvex::connect(std::size_t begin, std::vector<PolyConvex>& polyConvexList) {

    for (std::size_t i = begin + 1; i < polyConvexList.size(); ++i) {
        PolyConvex& pc1 = polyConvexList.at(i - 1);
        PolyConvex& pc2 = polyConvexList.at(i);

        pc1._adjacents.push_back(pc2._id);
        pc2._adjacents.push_back(pc1._id);

        PolygonTools::extendPolygon(pc1._geometry, pc2._geometry);

    }
}

PolyConvex::PolyConvex(Halfedge& he, std::size_t id, basic::LinearGradient& lgrad) :
        _supportHe(&he), _id(id) {
    EASY_FUNCTION();
    init(he.source()->point(), he.target()->point(), lgrad);
}

void PolyConvex::init(const Point_2& ps, const Point_2& pt, basic::LinearGradient& lgrad) {
    double t1 = lgrad.thickness(ps);
    double t2 = lgrad.thickness(pt);
    _geometry = PolygonTools::makeTrapeze(ps, pt, t1, t2);
    _originalTrapeze = _geometry;
    set_average_thickness((t1 + t2) / 2.);
}
PolyConvex::PolyConvex(const Point_2& ps, const Point_2& pt, std::size_t id, const Linear_polygon& geometry) :
        _geometry(geometry), _id(id) {
    _ps = ps;
    _pt = pt;
    _has_points = true;
    _originalTrapeze = _geometry;
}
PolyConvex::PolyConvex(const Point_2& ps, const Point_2& pt, std::size_t id, basic::LinearGradient& lgrad) :
        _supportHe(nullptr), _id(id) {
    EASY_FUNCTION();
    init(ps, pt, lgrad);
    _ps = ps;
    _pt = pt;
    _has_points = true;
}
} /* namespace laby */
