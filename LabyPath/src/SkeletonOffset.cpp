/*
 * SkeletonOffset.cpp
 *
 *  Created on: Jul 9, 2018
 *      Author: florian
 */

#include "SkeletonOffset.h"

#include "agg/agg_arc.h"
#include "basic/CircleIntersection.h"
#include "basic/RangeHelper.h"

namespace laby {

void SkeletonOffset::offset_face(const basic::HalfedgeNode& he, std::vector<Kernel::Segment_2>& result2, const double& offset_distance,
        std::unordered_map<const basic::SegmentNode*, IntersectType>& vertices_cache) {

    Kernel::Vector_2 vect(he.target()->point() - he.source()->point());
    Kernel::Vector_2 perp = vect.perpendicular(CGAL::Orientation::LEFT_TURN);

    if (CGAL::to_double(perp.squared_length()) == 0.) {
        std::cout << "error length =0" << std::endl;

    }
    perp *= offset_distance / std::sqrt(CGAL::to_double(perp.squared_length()));
    Kernel::Line_2 line(he.target()->point() + perp, vect);
    CGAL::Polygon_2<Kernel> poly;
    bool winding = false;
    Kernel::Point_2 last_point;
    for (const basic::HalfedgeNode& he2 : RangeHelper::make(he.next()->ccb())) {
        const auto& result = vertices_cache.try_emplace(&he2.curve(), //
                getLineSegmentIntersect(line, getSegment(he2))).first->second;
        addToSegmentsList(result, winding, result2, last_point);
    }
}

Kernel::Segment_2 SkeletonOffset::getSegment(const basic::HalfedgeNode& he2) {
    return Kernel::Segment_2(he2.source()->point(), he2.target()->point());
}

void SkeletonOffset::create_all_offsets(const double& distance, const basic::Arrangement_2Node& arr3, std::vector<Kernel::Segment_2>& result2) {
    std::size_t oldsize = result2.size() + 1;
    for (double offset_distance = distance / 2.; result2.size() != oldsize; offset_distance += distance) {
        oldsize = result2.size();
        SkeletonOffset::create_offset(arr3, offset_distance, result2);
    }
}

std::vector<CGAL::Polygon_with_holes_2<Kernel>> SkeletonOffset::get_polygon_offset(const std::vector<Kernel::Segment_2>& result2) {

    std::vector<Segment_info_2> segResult;
    for (const Kernel::Segment_2& seg : result2) {
        segResult.push_back(Segment_info_2(seg, EdgeInfo { }));
    }
    Arrangement_2 arr;
    CGAL::insert(arr, segResult.begin(), segResult.end());

    const Face & face_ubounded = *arr.unbounded_face();

    std::vector<CGAL::Polygon_with_holes_2<Kernel>> poly_hole;

    std::cout << " get_polygon_offset " << std::endl;

    for (auto iter = face_ubounded.inner_ccbs_begin(); iter != face_ubounded.inner_ccbs_end(); ++iter) {
        CGAL::Polygon_2<Kernel> outer;
        for (const Halfedge& he : RangeHelper::make((*iter)->twin()->ccb())) {
            outer.push_back(he.source()->point());
        }

        std::vector<CGAL::Polygon_2<Kernel> > holesP;
        const Face& polyFace = *(*iter)->twin()->face();
        for (auto iterHole = polyFace.inner_ccbs_begin(); iterHole != polyFace.inner_ccbs_end(); ++iterHole) {
            holesP.emplace_back();
            for (const Halfedge& he : RangeHelper::make(*iterHole)) {
                holesP.back().push_back(he.source()->point());
            }

        }
        poly_hole.emplace_back(outer, holesP.begin(), holesP.end());

    }

    return poly_hole;
}

void SkeletonOffset::create_offset(const basic::Arrangement_2Node& arr3, double offset_distance, std::vector<Kernel::Segment_2>& result2) {

    if (offset_distance <= 0) {
        for (const basic::HalfedgeNode& he : RangeHelper::make(arr3.edges_begin(), arr3.edges_end())) {
            if (he.curve().data().find(basic::EdgeNodeInfo(+1)) != he.curve().data().end()) {
                result2.emplace_back(he.source()->point(), he.target()->point());
            }
        }
        return;
    }
    std::vector<Kernel::Segment_2> offsetList;

    std::unordered_map<const basic::SegmentNode*, IntersectType> vertices_cache;

    for (const basic::HalfedgeNode& he : RangeHelper::make(arr3.edges_begin(), arr3.edges_end())) {
        // we meet polygon edge
        if (he.curve().data().find(basic::EdgeNodeInfo(+1)) != he.curve().data().end()) {
            // we must test if this is a hole
            if (he.next()->curve().data().find(basic::EdgeNodeInfo(+1)) != he.next()->curve().data().end()) {
                offset_face(*he.twin(), result2, offset_distance, vertices_cache);
                offset_corner(*he.twin(), result2, offset_distance, vertices_cache);

            } else {
                offset_face(he, result2, offset_distance, vertices_cache);
                offset_corner(he, result2, offset_distance, vertices_cache);
            }

        }

    }
}
void SkeletonOffset::addToSegmentsList(const IntersectType& result, bool& winding, std::vector<Kernel::Segment_2>& result2, Kernel::Point_2& last_point) {
    for (const Kernel::Point_2 s : result) {
        if (winding) {
            result2.emplace_back(last_point, s);
        }
        last_point = s;
        winding = !winding;
    }
}

SkeletonOffset::IntersectType SkeletonOffset::getLineSegmentIntersect(const Kernel::Line_2& line, const Kernel::Segment_2& seg) {

    IntersectType result;
    auto variant = CGAL::intersection(line, seg);
    if (variant) {
        if (const Kernel::Point_2* s = boost::get<Kernel::Point_2>(&*variant)) {
            result.emplace_back(*s);
        }
    }
    return result;
}
void SkeletonOffset::offset_corner(const basic::HalfedgeNode& he, std::vector<Kernel::Segment_2>& result2, const double& offset_distance,
        std::unordered_map<const basic::SegmentNode*, IntersectType>& vertices_cache) {

    const auto& o = *he.source();
    if (o.degree() < 4) {
        return;
    }
    basic::Arrangement_2Node::Halfedge_around_vertex_const_circulator circ = o.incident_halfedges();

    while (circ->curve().data().find(basic::EdgeNodeInfo(+1)) == circ->curve().data().end()) {
        ++circ;
    }

    while (circ->curve().data().find(basic::EdgeNodeInfo(+1)) != circ->curve().data().end()) {
        ++circ;
    }

    const basic::HalfedgeNode& he2 = *(circ->next());
    bool winding = false;
    Kernel::Point_2 last_point;
    std::vector<Kernel::Segment_2> arc_list;
    for (const basic::HalfedgeNode& he3 : RangeHelper::make(he2.ccb())) {

        const auto& result = vertices_cache.try_emplace(&he3.curve(), //
                basic::CircleIntersection::prob_2(o.point(), offset_distance, he3.source()->point(), he3.target()->point())).first->second;

        addToSegmentsList(result, winding, arc_list, last_point);

    }

    for (const Kernel::Segment_2& seg : arc_list) {
        agg::Arc arc(o.point(), offset_distance, seg.source(), seg.target());
        if (arc.getPoints().size() > 2) {
            result2.emplace_back(seg.source(), arc.getPoints().at(1));
            for (std::size_t i = 2; i < arc.getPoints().size() - 1; ++i) {
                const Kernel::Point_2& pa = arc.getPoints().at(i - 1);
                const Kernel::Point_2& pb = arc.getPoints().at(i);
                result2.emplace_back(pa, pb);
            }
            result2.emplace_back(arc.getPoints().at(arc.getPoints().size() - 2), seg.target());
        } else {
            result2.emplace_back(seg.source(), seg.target());
        }
    }
}

} /* namespace laby */
