/*
 * StrokeArrangement.cpp
 *
 *  Created on: Sep 7, 2018
 *      Author: florian
 */
#include "StrokeArrangement.h"
#include <CGAL/Distance_2/Point_2_Point_2.h>
#include <CGAL/Intersections_2/Line_2_Line_2.h>
#include <CGAL/Polygon_2.h>
#include <cstdint>
#include <utility>
#include <variant>

namespace laby::alter {

TrapezeEdgeInfo::TrapezeEdgeInfo(SourceTriplet source, TargetTriplet target, int32_t direction)
    : _direction(direction), _source(std::move(source.value)), _target(std::move(target.value)) {}

TrapezeEdgeInfo::TrapezeEdgeInfo() = default;

auto TrapezeEdgeInfo::intersection(const Kernel::Line_2& lineA,
                                   const Kernel::Line_2& lineB) -> Kernel::Point_2 {

    auto variant2 = CGAL::intersection(lineA, lineB);
    if (variant2) {
        if (const Kernel::Point_2* intersectionPoint = std::get_if<Kernel::Point_2>(&*variant2)) {
            return *intersectionPoint;
        }
    }
    return {0, 0};
}

void TrapezeEdgeInfo::computeGeometry(const Kernel::Point_2& firstBoundaryPoint,
                                      const Kernel::Point_2& secondBoundaryPoint,
                                      CGAL::Polygon_2<Kernel>& geometry) const {
    Kernel::Line_2 const centerLine(_source.origin(), _target.origin());
    Kernel::Line_2 const firstPerpendicular = centerLine.perpendicular(firstBoundaryPoint);
    Kernel::Line_2 const secondPerpendicular = centerLine.perpendicular(secondBoundaryPoint);
    Kernel::Line_2 const outerLine(_source.offset2(), _target.offset2());
    Kernel::Line_2 const innerLine(_source.offset1(), _target.offset1());
    geometry.push_back(intersection(innerLine, firstPerpendicular));
    geometry.push_back(intersection(innerLine, secondPerpendicular));
    geometry.push_back(intersection(outerLine, secondPerpendicular));
    geometry.push_back(intersection(outerLine, firstPerpendicular));
}

auto TrapezeEdgeInfo::getGeometry(const Kernel::Segment_2& segment) const
    -> CGAL::Polygon_2<Kernel> {
    CGAL::Polygon_2<Kernel> geometry;
    const Kernel::Point_2& source = segment.source();
    const Kernel::Point_2& target = segment.target();
    const Kernel::FT sourceDistance = CGAL::squared_distance(_source.origin(), source);
    const Kernel::FT targetDistance = CGAL::squared_distance(_source.origin(), target);
    if (sourceDistance < targetDistance) {

        computeGeometry(source, target, geometry);
    } else {
        computeGeometry(target, source, geometry);
    }
    return geometry;
}

} /* namespace laby::alter */
