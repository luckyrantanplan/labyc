/*
 * SkeletonOffset.cpp
 *
 *  Created on: Jul 9, 2018
 *      Author: florian
 */

#include "SkeletonOffset.h"

#include <CGAL/Arrangement_2/Arrangement_on_surface_2_global.h>
#include <CGAL/Intersections_2/Line_2_Segment_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/enum.h>
#include <CGAL/number_utils.h>

#include <GeomData.h>
#include <basic/RangeHelper.h>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <unordered_map>
#include <variant>
#include <vector>

#include "agg/agg_arc.h"
#include "basic/AugmentedPolygonSet.h"
#include "basic/CircleIntersection.h"

namespace laby {

namespace {

constexpr double kHalfDistanceDivisor = 2.0;

} // namespace

auto SkeletonOffset::offsetFace(
    const basic::HalfedgeNode& halfedge, std::vector<Kernel::Segment_2>& resultSegments,
    const double& offsetDistance,
    std::unordered_map<const basic::SegmentNode*, IntersectType>& verticesCache) -> void {

    Kernel::Vector_2 const vector(halfedge.target()->point() - halfedge.source()->point());
    Kernel::Vector_2 perpendicular = vector.perpendicular(CGAL::LEFT_TURN);

    if (CGAL::to_double(perpendicular.squared_length()) == 0.0) {
        std::cout << "error length =0\n";
    }

    perpendicular *= offsetDistance / std::sqrt(CGAL::to_double(perpendicular.squared_length()));
    Kernel::Line_2 const line(halfedge.target()->point() + perpendicular, vector);
    bool winding = false;
    Kernel::Point_2 lastPoint;
    for (const basic::HalfedgeNode& currentHalfedge : RangeHelper::make(halfedge.next()->ccb())) {
        const auto& intersectionPoints =
            verticesCache
                .try_emplace(&currentHalfedge.curve(),
                             getLineSegmentIntersect(line, getSegment(currentHalfedge)))
                .first->second;
        addToSegmentsList(intersectionPoints, winding, resultSegments, lastPoint);
    }
}

auto SkeletonOffset::getSegment(const basic::HalfedgeNode& halfedge) -> Kernel::Segment_2 {
    return {halfedge.source()->point(), halfedge.target()->point()};
}

auto SkeletonOffset::createAllOffsets(const double& distance,
                                      const basic::Arrangement_2Node& arrangement,
                                      std::vector<Kernel::Segment_2>& resultSegments) -> void {
    std::size_t previousSize = resultSegments.size() + 1;
    double offsetDistance = distance / kHalfDistanceDivisor;
    while (resultSegments.size() != previousSize) {
        previousSize = resultSegments.size();
        SkeletonOffset::createOffset(arrangement, offsetDistance, resultSegments);
        offsetDistance += distance;
    }
}

auto SkeletonOffset::getPolygonOffset(const std::vector<Kernel::Segment_2>& resultSegments)
    -> std::vector<CGAL::Polygon_with_holes_2<Kernel>> {

    std::vector<Segment_info_2> segResult;
    segResult.reserve(resultSegments.size());
    for (const Kernel::Segment_2& segment : resultSegments) {
        segResult.emplace_back(segment, EdgeInfo{});
    }

    Arrangement_2 arr;
    CGAL::insert(arr, segResult.begin(), segResult.end());

    const Face& unboundedFace = *arr.unbounded_face();

    std::vector<CGAL::Polygon_with_holes_2<Kernel>> polygonWithHoles;

    std::cout << " get_polygon_offset \n";

    for (auto outerIterator = unboundedFace.inner_ccbs_begin();
         outerIterator != unboundedFace.inner_ccbs_end(); ++outerIterator) {
        CGAL::Polygon_2<Kernel> outer;
        for (const Halfedge& halfedge : RangeHelper::make((*outerIterator)->twin()->ccb())) {
            outer.push_back(halfedge.source()->point());
        }

        std::vector<CGAL::Polygon_2<Kernel>> holes;
        const Face& polygonFace = *(*outerIterator)->twin()->face();
        for (auto holeIterator = polygonFace.inner_ccbs_begin();
             holeIterator != polygonFace.inner_ccbs_end(); ++holeIterator) {
            holes.emplace_back();
            for (const Halfedge& halfedge : RangeHelper::make(*holeIterator)) {
                holes.back().push_back(halfedge.source()->point());
            }
        }

        polygonWithHoles.emplace_back(outer, holes.begin(), holes.end());
    }

    return polygonWithHoles;
}

auto SkeletonOffset::createOffset(const basic::Arrangement_2Node& arrangement,
                                  double offsetDistance,
                                  std::vector<Kernel::Segment_2>& resultSegments) -> void {

    if (offsetDistance <= 0.0) {
        for (const basic::HalfedgeNode& halfedge :
             RangeHelper::make(arrangement.edges_begin(), arrangement.edges_end())) {
            if (basic::edgeHasPolygonId(halfedge, +1)) {
                resultSegments.emplace_back(halfedge.source()->point(), halfedge.target()->point());
            }
        }
        return;
    }

    std::unordered_map<const basic::SegmentNode*, IntersectType> verticesCache;

    for (const basic::HalfedgeNode& halfedge :
         RangeHelper::make(arrangement.edges_begin(), arrangement.edges_end())) {
        if (basic::edgeHasPolygonId(halfedge, +1)) {
            if (basic::edgeHasPolygonId(*halfedge.next(), +1)) {
                offsetFace(*halfedge.twin(), resultSegments, offsetDistance, verticesCache);
                offsetCorner(*halfedge.twin(), resultSegments, offsetDistance, verticesCache);
            } else {
                offsetFace(halfedge, resultSegments, offsetDistance, verticesCache);
                offsetCorner(halfedge, resultSegments, offsetDistance, verticesCache);
            }
        }
    }
}

auto SkeletonOffset::addToSegmentsList(const IntersectType& intersectionPoints, bool& winding,
                                       std::vector<Kernel::Segment_2>& resultSegments,
                                       Kernel::Point_2& lastPoint) -> void {
    for (const Kernel::Point_2& point : intersectionPoints) {
        if (winding) {
            resultSegments.emplace_back(lastPoint, point);
        }
        lastPoint = point;
        winding = !winding;
    }
}

auto SkeletonOffset::getLineSegmentIntersect(
    const Kernel::Line_2& line, const Kernel::Segment_2& segment) -> SkeletonOffset::IntersectType {

    IntersectType result;
    auto variant = CGAL::intersection(line, segment);
    if (variant) {
        if (const Kernel::Point_2* point = std::get_if<Kernel::Point_2>(&*variant)) {
            result.emplace_back(*point);
        }
    }
    return result;
}

auto SkeletonOffset::offsetCorner(
    const basic::HalfedgeNode& halfedge, std::vector<Kernel::Segment_2>& resultSegments,
    const double& offsetDistance,
    std::unordered_map<const basic::SegmentNode*, IntersectType>& verticesCache) -> void {

    const auto& originVertex = *halfedge.source();
    if (originVertex.degree() < 4) {
        return;
    }
    basic::Arrangement_2Node::Halfedge_around_vertex_const_circulator circulator =
        originVertex.incident_halfedges();

    while (!basic::edgeHasPolygonId(*circulator, +1)) {
        ++circulator;
    }

    while (basic::edgeHasPolygonId(*circulator, +1)) {
        ++circulator;
    }

    const basic::HalfedgeNode& cornerHalfedge = *(circulator->next());
    bool winding = false;
    Kernel::Point_2 lastPoint;
    std::vector<Kernel::Segment_2> arcList;
    for (const basic::HalfedgeNode& ringHalfedge : RangeHelper::make(cornerHalfedge.ccb())) {

        const auto& intersectionPoints =
            verticesCache
                .try_emplace(&ringHalfedge.curve(),
                             basic::CircleIntersection::prob2(originVertex.point(), offsetDistance,
                                                              ringHalfedge.source()->point(),
                                                              ringHalfedge.target()->point()))
                .first->second;

        addToSegmentsList(intersectionPoints, winding, arcList, lastPoint);
    }

    for (const Kernel::Segment_2& segment : arcList) {
        agg::Arc const arc(originVertex.point(), offsetDistance, segment.source(),
                           segment.target());
        if (arc.getPoints().size() > 2) {
            resultSegments.emplace_back(segment.source(), arc.getPoints().at(1));
            for (std::size_t i = 2; i < arc.getPoints().size() - 1; ++i) {
                const Kernel::Point_2& pointA = arc.getPoints().at(i - 1);
                const Kernel::Point_2& pointB = arc.getPoints().at(i);
                resultSegments.emplace_back(pointA, pointB);
            }
            resultSegments.emplace_back(arc.getPoints().at(arc.getPoints().size() - 2),
                                        segment.target());
        } else {
            resultSegments.emplace_back(segment.source(), segment.target());
        }
    }
}

} /* namespace laby */
