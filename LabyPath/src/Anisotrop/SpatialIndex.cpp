/*
 * SpatialIndex.cpp
 *
 *  Created on: Apr 3, 2018
 *      Author: florian
 */

#include "SpatialIndex.h"

#include <algorithm>
#include <cmath>

#include <boost/geometry/index/predicates.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/tuple/detail/tuple_basic.hpp>
// #include <CGAL/Arr_dcel_base.h>
#include "basic/EasyProfilerCompat.h"
#include <CGAL/Arr_geometry_traits/Curve_data_aux.h>
#include <CGAL/Bbox_2.h>
#include <CGAL/Kernel/global_functions_2.h>
#include <CGAL/Point_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Vector_2.h>
#include <CGAL/number_utils.h>
#include <CGAL/squared_distance_2.h>

#include "Net.h"
#include "QueueCost.h"

namespace laby::aniso {

namespace {

constexpr double kBlockingCosineThreshold = 0.5;

} // namespace

SpatialIndex::SpatialIndex(std::vector<PolyConvex>& polyConvexList)
    : _polyConvexList(&polyConvexList) {
    EASY_FUNCTION();
}

void SpatialIndex::insert(const PolyConvex& polyConvex) {
    EASY_FUNCTION();
    CGAL::Bbox_2 bbox = polyConvex._geometry.bbox();
    _rtree.insert(Value{getBox(bbox), polyConvex._id});
}

auto SpatialIndex::getBox(const CGAL::Bbox_2& bbox) -> Box {

    return Box{Point{bbox.xmin(), bbox.ymin()}, Point{bbox.xmax(), bbox.ymax()}};
}

auto SpatialIndex::getApproxSeg(const PolyConvex& polyConvex) -> CGAL::Segment_2<KApprox> {
    const CGAL::Segment_2<Kernel>& segment = polyConvex._supportHe->curve();
    CGAL::Point_2<KApprox> sourcePoint(CGAL::to_double(segment.source().x()),
                                       CGAL::to_double(segment.source().y()));
    CGAL::Point_2<KApprox> targetPoint(CGAL::to_double(segment.target().x()),
                                       CGAL::to_double(segment.target().y()));
    return {sourcePoint, targetPoint};
}

auto SpatialIndex::isPointInside(const PolyConvex& polyConvex, const Vertex& vertex,
                                 uint32_t& index) const -> bool {
    EASY_FUNCTION();

    CGAL::Bbox_2 bbox = vertex.point().bbox();

    return _rtree.qbegin(boost::geometry::index::intersects(getBox(bbox)) && //
                         boost::geometry::index::satisfies([&polyConvex, &vertex, &index,
                                                            this](const Value& value) {
                             EASY_BLOCK("isPointInsideCallback");

                             const PolyConvex& otherPolyConvex = _polyConvexList->at(value.second);

                             const std::vector<std::size_t>& adjacentPolyConvexes =
                                 polyConvex._adjacents;
                             if (otherPolyConvex._id == polyConvex._id ||
                                 std::find(adjacentPolyConvexes.begin(), adjacentPolyConvexes.end(),
                                           otherPolyConvex._id) != adjacentPolyConvexes.end()) {
                                 return false;
                             }

                             if (otherPolyConvex._geometry.has_on_bounded_side(vertex.point()) ||
                                 otherPolyConvex._geometry.has_on_boundary(vertex.point())) {
                                 index = value.second;
                                 return true;
                             }
                             return false;
                         })) != _rtree.qend();
}

auto SpatialIndex::areSegmentsCloseEnough(const CGAL::Segment_2<KApprox>& segment,
                                          const CGAL::Segment_2<KApprox>& otherSegment,
                                          double thickness) -> bool {

    return CGAL::squared_distance(segment, otherSegment) < (thickness * thickness);
}

auto SpatialIndex::cosinus(const CGAL::Segment_2<KApprox>& segment,
                           const CGAL::Segment_2<KApprox>& otherSegment) -> double {
    CGAL::Vector_2<KApprox> firstVector = segment.to_vector();
    CGAL::Vector_2<KApprox> secondVector = otherSegment.to_vector();
    firstVector = firstVector / std::sqrt(firstVector.squared_length());
    secondVector = secondVector / std::sqrt(secondVector.squared_length());
    return firstVector * secondVector;
}

auto SpatialIndex::testIsSamePin(const Halfedge& halfedge, const Net& net) -> bool {
    const Vertex& sourceVertex = *halfedge.source();
    const Vertex& targetVertex = *halfedge.target();
    const laby::Vertex& netSourceVertex = net.source().vertex();
    const laby::Vertex& netTargetVertex = net.target().vertex();

    if (sourceVertex.data().getType() == VertexInfo::PIN) {
        if (&sourceVertex == &netSourceVertex || &sourceVertex == &netTargetVertex) {
            return true;
        }
    }
    if (targetVertex.data().getType() == VertexInfo::PIN) {
        if (&targetVertex == &netSourceVertex || &targetVertex == &netTargetVertex) {
            return true;
        }
    }
    return false;
}

auto SpatialIndex::testPinProximity(const CGAL::Segment_2<KApprox>& segment, const Vertex& vertex,
                                    const Net& /*net*/, double thickness) -> bool {
    if (vertex.data().getType() == VertexInfo::PIN) {

        CGAL::Point_2<KApprox> point(CGAL::to_double(vertex.point().x()),
                                     CGAL::to_double(vertex.point().y()));
        if (CGAL::to_double(CGAL::squared_distance(point, segment)) <= thickness * thickness) {
            return true;
        }
    }
    return false;
}

auto SpatialIndex::testIntersectionTest(const PolyConvex& polyConvex,
                                        const PolyConvex& otherPolyConvex) -> bool {
    const CGAL::Segment_2<KApprox> otherSegment = getApproxSeg(otherPolyConvex);
    double otherThickness = otherPolyConvex.thickness();
    const CGAL::Segment_2<KApprox> segment = getApproxSeg(polyConvex);
    double thickness = polyConvex.thickness();

    return areSegmentsCloseEnough(segment, otherSegment, (otherThickness + thickness) / 2);
}

auto SpatialIndex::intersectionTest(QueueCost& cost, const PolyConvex& /*polyConvex*/,
                                    const Net& net, const std::unordered_set<int32_t>& targetNets,
                                    const CGAL::Segment_2<KApprox>& segment, double thickness,
                                    const Value& value) const -> bool {
    EASY_FUNCTION();
    const PolyConvex& otherPolyConvex = _polyConvexList->at(value.second);
    //    if (pc2._supportHe->curve().data().direction() == EdgeInfo::CELL &&
    //    pc._supportHe->curve().data().direction() == EdgeInfo::CELL) {
    //        return false;
    //    }
    const CGAL::Segment_2<KApprox> otherSegment = getApproxSeg(otherPolyConvex);
    double otherThickness = otherPolyConvex.thickness();
    double minSeparation = (otherThickness + thickness) / 2;
    if (areSegmentsCloseEnough(segment, otherSegment, minSeparation)) {

        const Vertex& sourceVertex = *otherPolyConvex._supportHe->source();
        const Vertex& targetVertex = *otherPolyConvex._supportHe->target();

        if (testIsSamePin(*otherPolyConvex._supportHe, net)) {

            return false;
        }

        if (testPinProximity(segment, sourceVertex, net, minSeparation) ||
            testPinProximity(segment, targetVertex, net, minSeparation)) {
            ++cost.congestion;

            return true;
        }

        double cosineValue = cosinus(segment, otherSegment);
        // <45 degree : true if we should block
        if (cosineValue * cosineValue > kBlockingCosineThreshold) {
            int32_t netId = otherPolyConvex._supportHe->curve().data().getNet();
            if (cost.memory_source.count(netId) != 0U) {
                cost.future_memory_source.emplace(netId);
            } else if (targetNets.count(netId) != 0U) {
                cost.future_memory_target.emplace(netId);
            } else {

                ++cost.congestion;
            }

            return true;
        }
    }

    return false;
}

void SpatialIndex::updateCostIfIntersect(const PolyConvex& polyConvex, const Net& net,
                                         const std::unordered_set<int32_t>& targetNets,
                                         QueueCost& cost) const {
    EASY_FUNCTION();

    CGAL::Bbox_2 bbox = polyConvex._geometry.bbox();
    const CGAL::Segment_2<KApprox> segment = getApproxSeg(polyConvex);
    double thickness = polyConvex.thickness();

    _rtree.qbegin(
        boost::geometry::index::intersects(getBox(bbox)) && //
        boost::geometry::index::satisfies([&cost, &polyConvex, &net, &targetNets, &segment,
                                           &thickness, this](const Value& value) {
            return intersectionTest(cost, polyConvex, net, targetNets, segment, thickness, value);
        }));
}

} // namespace laby::aniso
