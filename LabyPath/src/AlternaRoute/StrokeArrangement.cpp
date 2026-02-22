/*
 * StrokeArrangement.cpp
 *
 *  Created on: Sep 7, 2018
 *      Author: florian
 */
#include "StrokeArrangement.h"

namespace laby {
namespace alter {

TrapezeEdgeInfo::TrapezeEdgeInfo(const Offset_triplet& source, const Offset_triplet& target, const int32_t& direction) :
        _direction(direction), _source(source), _target(target) {

}

TrapezeEdgeInfo::TrapezeEdgeInfo() {
}

const Kernel::Point_2 TrapezeEdgeInfo::intersection(const Kernel::Line_2& l, const Kernel::Line_2& h) {

    auto variant2 = CGAL::intersection(l, h);
    if (variant2) {
        if (const Kernel::Point_2* p2 = boost::get<Kernel::Point_2>(&*variant2)) {
            return *p2;
        }

    }
    return Kernel::Point_2(0, 0);
}

void TrapezeEdgeInfo::computeGeometry(const Kernel::Point_2& source, const Kernel::Point_2& target, CGAL::Polygon_2<Kernel>& geometry) const {
    Kernel::Line_2 l(_source.origin, _target.origin);
    Kernel::Line_2 h1 = l.perpendicular(source);
    Kernel::Line_2 h2 = l.perpendicular(target);
    Kernel::Line_2 ab(_source.offset2, _target.offset2);
    Kernel::Line_2 dc(_source.offset1, _target.offset1);
    geometry.push_back(intersection(dc, h1));
    geometry.push_back(intersection(dc, h2));
    geometry.push_back(intersection(ab, h2));
    geometry.push_back(intersection(ab, h1));
}

const CGAL::Polygon_2<Kernel> TrapezeEdgeInfo::getGeometry(const Kernel::Segment_2& seg) const {
    CGAL::Polygon_2<Kernel> geometry;
    const Kernel::Point_2& source = seg.source();
    const Kernel::Point_2& target = seg.target();
    if (CGAL::compare_distance_to_point(_source.origin, source, target) == CGAL::SMALLER) {

        computeGeometry(source, target, geometry);
    } else {
        computeGeometry(target, source, geometry);
    }
    return geometry;
}

}
/* namespace basic */
} /* namespace laby */
