/*
 * StrokeArrangement.cpp
 *
 *  Created on: Sep 7, 2018
 *      Author: florian
 */
#include "StrokeArrangement.h"

namespace laby::alter {

TrapezeEdgeInfo::TrapezeEdgeInfo(SourceTriplet source, TargetTriplet target, int32_t direction)
    : _direction(direction), _source(std::move(source.value)), _target(std::move(target.value)) {}

TrapezeEdgeInfo::TrapezeEdgeInfo() = default;

auto TrapezeEdgeInfo::intersection(const Kernel::Line_2& lineA,
                                   const Kernel::Line_2& lineB) -> Kernel::Point_2 {

    auto variant2 = CGAL::intersection(lineA, lineB);
    if (variant2) {
        if (const Kernel::Point_2* intersectionPoint = boost::get<Kernel::Point_2>(&*variant2)) {
            return *intersectionPoint;
        }
    }
    return {0, 0};
}

void TrapezeEdgeInfo::computeGeometry(const Kernel::Point_2& firstBoundaryPoint,
                                      const Kernel::Point_2& secondBoundaryPoint,
                                      CGAL::Polygon_2<Kernel>& geometry) const {
    Kernel::Line_2 centerLine(_source.origin(), _target.origin());
    Kernel::Line_2 firstPerpendicular = centerLine.perpendicular(firstBoundaryPoint);
    Kernel::Line_2 secondPerpendicular = centerLine.perpendicular(secondBoundaryPoint);
    Kernel::Line_2 outerLine(_source.offset2(), _target.offset2());
    Kernel::Line_2 innerLine(_source.offset1(), _target.offset1());
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
    if (CGAL::compare_distance_to_point(_source.origin(), source, target) == CGAL::SMALLER) {

        computeGeometry(source, target, geometry);
    } else {
        computeGeometry(target, source, geometry);
    }
    return geometry;
}

} /* namespace laby::alter */
