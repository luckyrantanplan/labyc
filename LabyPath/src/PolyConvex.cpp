/*
 * PolyConvex.cpp
 *
 *  Created on: Mar 15, 2018
 *      Author: florian
 */

#include "PolyConvex.h"

#include <cstddef>

#include "basic/EasyProfilerCompat.h"
#include <utility>

#include "basic/LinearGradient.h"

namespace laby {

namespace {

constexpr double kAverageThicknessDivisor = 2.0;

} // namespace

static auto PolyConvex::testConvexPolyIntersect(const Linear_polygon& polygon,
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

static auto PolyConvex::connect(std::size_t first, std::size_t second,
                                std::vector<PolyConvex>& polyConvexList,
                                const Point_2& center) -> void {
    // we push in the vector, before to take reference on it (otherwise we get memory corruption)
    polyConvexList.emplace_back();

    PolyConvex& pc1 = polyConvexList.at(first);
    PolyConvex& pc2 = polyConvexList.at(second);

    pc1._adjacents.push_back(pc2._id);
    pc2._adjacents.push_back(pc1._id);

    PolyConvex& triangle = polyConvexList.back();
    triangle._id = polyConvexList.size() - 1;
    triangle._geometry =
        PolygonTools::createJoinTriangle(pc1._originalTrapeze, pc2._originalTrapeze, center);

    triangle._adjacents.push_back(pc2._id);
    triangle._adjacents.push_back(pc1._id);
    pc1._adjacents.push_back(triangle._id);
    pc2._adjacents.push_back(triangle._id);
}

static auto PolyConvex::connect(std::size_t first, std::size_t second,
                                std::vector<PolyConvex>& polyConvexList) -> void {
    PolyConvex& pc1 = polyConvexList.at(first);
    PolyConvex& pc2 = polyConvexList.at(second);

    pc1._adjacents.push_back(pc2._id);
    pc2._adjacents.push_back(pc1._id);
}

static auto PolyConvex::connect(std::size_t begin,
                                std::vector<PolyConvex>& polyConvexList) -> void {

    for (std::size_t i = begin + 1; i < polyConvexList.size(); ++i) {
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
    init(halfedge.source()->point(), halfedge.target()->point(), gradient);
}

auto PolyConvex::init(const Point_2& sourcePoint, const Point_2& targetPoint,
                      basic::LinearGradient& gradient) -> void {
    const double sourceThickness = gradient.thickness(sourcePoint);
    const double targetThickness = gradient.thickness(targetPoint);
    _geometry =
        PolygonTools::makeTrapeze(sourcePoint, targetPoint, sourceThickness, targetThickness);
    _originalTrapeze = _geometry;
    setAverageThickness((sourceThickness + targetThickness) / kAverageThicknessDivisor);
}
PolyConvex::PolyConvex(Point_2 sourcePoint, Point_2 targetPoint, std::size_t polygonId,
                       Linear_polygon geometry)
    : _geometry(std::move(geometry)), _originalTrapeze(_geometry), _id(polygonId),
      _ps(std::move(sourcePoint)), _pt(std::move(targetPoint)), _has_points(true) {}

PolyConvex::PolyConvex(const Point_2& sourcePoint, const Point_2& targetPoint,
                       std::size_t polygonId, basic::LinearGradient& gradient)
    : _id(polygonId), _ps(sourcePoint), _pt(targetPoint), _has_points(true) {
    EASY_FUNCTION();
    init(sourcePoint, targetPoint, gradient);
}
} /* namespace laby */
