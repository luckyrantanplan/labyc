/*
 * PolyConvex.cpp
 *
 *  Created on: Mar 15, 2018
 *      Author: florian
 */

#include "PolyConvex.h"

#include <CGAL/Intersections_2/Segment_2_Segment_2.h>
#include <CGAL/Segment_2.h>
#include <GeomData.h>
#include <basic/PolygonTools.h>
#include <basic/RangeHelper.h>
#include <cstddef>

#include "basic/EasyProfilerCompat.h"
#include <utility>
#include <vector>

#include "basic/LinearGradient.h"

namespace laby {

namespace {

constexpr double kAverageThicknessDivisor = 2.0;

} // namespace

auto PolyConvex::testConvexPolyIntersect(const Linear_polygon& polygon,
                                         const Linear_polygon& otherPolygon) -> bool {
    EASY_FUNCTION();

    for (const CGAL::Segment_2<Kernel>& edge :
         RangeHelper::make(polygon.edges_begin(), polygon.edges_end())) {
        for (const CGAL::Segment_2<Kernel>& otherEdge :
             RangeHelper::make(otherPolygon.edges_begin(), otherPolygon.edges_end())) {
            if (CGAL::do_intersect(edge, otherEdge)) {
                return true;
            }
        }
    }

    if (polygon.has_on_bounded_side(otherPolygon.vertex(0))) {
        return true;
    }
    if (otherPolygon.has_on_bounded_side(polygon.vertex(0))) {
        return true;
    }
    return false;
}

auto PolyConvex::connect(const PolyConvexConnection& connection,
                         std::vector<PolyConvex>& polyConvexList, const Point_2& center) -> void {
    // we push in the vector, before to take reference on it (otherwise we get memory corruption)
    polyConvexList.emplace_back();

    PolyConvex& firstPolyConvex = polyConvexList.at(connection.firstPolyConvexId);
    PolyConvex& secondPolyConvex = polyConvexList.at(connection.secondPolyConvexId);

    firstPolyConvex._adjacents.push_back(secondPolyConvex._id);
    secondPolyConvex._adjacents.push_back(firstPolyConvex._id);

    PolyConvex& triangle = polyConvexList.back();
    triangle._id = polyConvexList.size() - 1;
    triangle._geometry = PolygonTools::createJoinTriangle(
        firstPolyConvex._originalTrapeze, secondPolyConvex._originalTrapeze, center);

    triangle._adjacents.push_back(secondPolyConvex._id);
    triangle._adjacents.push_back(firstPolyConvex._id);
    firstPolyConvex._adjacents.push_back(triangle._id);
    secondPolyConvex._adjacents.push_back(triangle._id);
}

auto PolyConvex::connect(const PolyConvexConnection& connection,
                         std::vector<PolyConvex>& polyConvexList) -> void {
    PolyConvex& firstPolyConvex = polyConvexList.at(connection.firstPolyConvexId);
    PolyConvex& secondPolyConvex = polyConvexList.at(connection.secondPolyConvexId);

    firstPolyConvex._adjacents.push_back(secondPolyConvex._id);
    secondPolyConvex._adjacents.push_back(firstPolyConvex._id);
}

auto PolyConvex::connect(std::size_t beginIndex, std::vector<PolyConvex>& polyConvexList) -> void {

    for (std::size_t i = beginIndex + 1; i < polyConvexList.size(); ++i) {
        PolyConvex& pc1 = polyConvexList.at(i - 1);
        PolyConvex& pc2 = polyConvexList.at(i);

        pc1._adjacents.push_back(pc2._id);
        pc2._adjacents.push_back(pc1._id);

        PolygonTools::extendPolygon(pc1._geometry, pc2._geometry);
    }
}

PolyConvex::PolyConvex(Halfedge& halfedge, std::size_t polygonId, basic::LinearGradient& gradient)
    : _supportHe(&halfedge), _id(polygonId) {
    EASY_FUNCTION();
    init(PolyConvexEndpoints{halfedge.source()->point(), halfedge.target()->point()}, gradient);
}

auto PolyConvex::init(const PolyConvexEndpoints& endpoints,
                      basic::LinearGradient& gradient) -> void {
    const double sourceThickness = gradient.thickness(endpoints.sourcePoint);
    const double targetThickness = gradient.thickness(endpoints.targetPoint);
    _geometry = PolygonTools::makeTrapeze(endpoints.sourcePoint, endpoints.targetPoint,
                                          sourceThickness, targetThickness);
    _originalTrapeze = _geometry;
    setAverageThickness((sourceThickness + targetThickness) / kAverageThicknessDivisor);
}
PolyConvex::PolyConvex(PolyConvexEndpoints endpoints, std::size_t polygonId,
                       Linear_polygon geometry)
    : _geometry(std::move(geometry)), _originalTrapeze(_geometry), _id(polygonId),
      _sourcePoint(std::move(endpoints.sourcePoint)),
      _targetPoint(std::move(endpoints.targetPoint)), _has_points(true) {}

PolyConvex::PolyConvex(const PolyConvexEndpoints& endpoints, std::size_t polygonId,
                       basic::LinearGradient& gradient)
    : _id(polygonId), _sourcePoint(endpoints.sourcePoint), _targetPoint(endpoints.targetPoint),
      _has_points(true) {
    EASY_FUNCTION();
    init(endpoints, gradient);
}
} /* namespace laby */
