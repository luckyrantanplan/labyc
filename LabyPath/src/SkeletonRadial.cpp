/*
 * SkeletonRadial.cpp
 *
 *  Created on: Jul 9, 2018
 *      Author: florian
 */

#include "SkeletonRadial.h"

#include <CGAL/Intersections_2/Line_2_Segment_2.h>
#include <CGAL/enum.h>
#include <CGAL/number_utils.h>
#include <boost/variant/get.hpp>

#include "basic/RangeHelper.h"
#include <CGAL/box_intersection_d.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

namespace laby {

auto SkeletonRadial::registerFace(
    const basic::HalfedgeNode& he,
    std::unordered_map<const basic::FaceNode*, FaceHelper>& vertices_cache) const -> void {
    {
        FaceHelper& faceHelper =
            vertices_cache.try_emplace(&*he.face(), FaceHelper()).first->second;
        faceHelper._perp =
            (he.curve().target() - he.curve().source()).perpendicular(CGAL::LEFT_TURN);
        if (faceHelper._type != FaceHelper::Type::Unknown) {
            std::cout << "face lateral already exists with " << static_cast<int>(faceHelper._type)
                      << '\n';
        }
        faceHelper._type = FaceHelper::Type::Lateral;
    }
    const auto& o = *he.source();
    if (o.degree() < 4) {
        return;
    }
    basic::Arrangement_2Node::Halfedge_around_vertex_const_circulator circ = o.incident_halfedges();

    while (!basic::edgeHasPolygonId(*circ, +1)) {
        ++circ;
    }

    while (basic::edgeHasPolygonId(*circ, +1)) {
        ++circ;
    }

    const basic::HalfedgeNode& heSecond = *circ;
    const basic::HalfedgeNode& heFirst = *heSecond.next();

    if (&*heSecond.face() != &*heFirst.face()) {
        std::cout << "problem of face" << std::endl;
    }
    FaceHelper& faceHelper =
        vertices_cache.try_emplace(&*heSecond.face(), FaceHelper()).first->second;

    if (faceHelper._type != FaceHelper::Type::Unknown) {
        std::cout << "face corner already exists with " << static_cast<int>(faceHelper._type)
                  << '\n';
    }

    faceHelper._type = FaceHelper::Type::Corner;
    faceHelper._o = o.point();
}

auto SkeletonRadial::traverseCorner(const Incidence& incidence, const FaceHelper& faceHelper,
                                    std::vector<Kernel::Point_2>& result) const -> Incidence {

    Kernel::Line_2 line = Kernel::Line_2(incidence._test, faceHelper._o);

    for (const basic::HalfedgeNode& he2 : RangeHelper::make(incidence._hedge->ccb())) {
        auto variant2 = CGAL::intersection(line, Kernel::Segment_2(he2.curve()));
        if (variant2) {
            if (const Kernel::Point_2* p2 = boost::get<Kernel::Point_2>(&*variant2)) {
                if (CGAL::squared_distance(*p2, faceHelper._o) > 0. and //
                    CGAL::squared_distance(*p2, incidence._test) > 0) {
                    if (filterNewVertex(result, *p2)) {
                        result.emplace_back(*p2);
                        return Incidence(*p2, *he2.twin());
                    }
                }
            } else {
                std::cout << " variant 2 is not a point ";
            }
        }
    }

    result.emplace_back(faceHelper._o);
    return Incidence();
}

auto SkeletonRadial::traverseRay(const basic::HalfedgeNode& hedge, const Kernel::Point_2& test,
                                 const Kernel::Vector_2& dir,
                                 std::vector<Kernel::Point_2>& result) const -> Incidence {

    Kernel::Line_2 line(test, dir);
    for (const basic::HalfedgeNode& he : RangeHelper::make(hedge.next()->ccb())) {
        auto variant = CGAL::intersection(line, Kernel::Segment_2(he.curve()));
        if (variant) {
            if (const Kernel::Point_2* s = boost::get<Kernel::Point_2>(&*variant)) {
                if (*s != test) {
                    if (filterNewVertex(result, *s)) {
                        result.emplace_back(*s);
                        return Incidence(*s, *he.twin());
                    } else {
                        return Incidence();
                    }
                }
            } else {
                std::cout << "variant is not a point" << std::endl;
            }
        }
    }
    std::cout << "test ray is " << test << std::endl;
    for (const basic::HalfedgeNode& he : RangeHelper::make(hedge.next()->ccb())) {
        std::cout << he.curve() << std::endl;
    }
    std::cout << "no incidence traverse_ray!" << std::endl;
    return Incidence();
}

auto SkeletonRadial::traverseFace(
    const Incidence& incidence,
    const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache, //
    std::vector<Kernel::Point_2>& result) const -> SkeletonRadial::Incidence {
    auto ite = faceCache.find(&*incidence._hedge->face());

    if (ite == faceCache.end()) {
        return Incidence();
    }
    const FaceHelper& faceHelper = ite->second;
    switch (faceHelper._type) {
    case FaceHelper::Type::Lateral: {
        // to be calculated at the beginning, at the creation of FaceHelper

        return traverseRay(*incidence._hedge, incidence._test, faceHelper._perp, result);

        break;
    }
    case FaceHelper::Type::Corner: {
        Kernel::Line_2 line;
        static_cast<void>(line);

        return traverseCorner(incidence, faceHelper, result);

        break;
    }
    case FaceHelper::Type::Unknown: {
        std::cout << "Unknow faced " << std::endl;
        break;
    }
    }
    return Incidence();
}

auto SkeletonRadial::filterNewVertex(const std::vector<Kernel::Point_2>& result,
                                     const Kernel::Point_2& vertex) const -> bool {
    if (result.size() < 2) {
        return true;
    }
    const Kernel::Segment_2 new_seg(result.at(result.size() - 1), vertex);
    for (std::size_t i = result.size() - 1; i > 0; --i) {
        const Kernel::Segment_2 seg(result.at(i - 1), result.at(i));

        double cos_theta =
            CGAL::to_double(CGAL::scalar_product(new_seg.to_vector(), seg.to_vector())) / //
            (sqrt(CGAL::to_double(new_seg.squared_length())) *
             sqrt(CGAL::to_double(seg.squared_length())));

        if (cos_theta * cos_theta > _config.cosTheta * _config.cosTheta) {
            if (CGAL::squared_distance(new_seg, seg) <
                _config.filtered_distance * _config.filtered_distance) {
                const Kernel::Point_2 pa = seg.supporting_line().projection(new_seg.source());
                const Kernel::Point_2 pb = seg.supporting_line().projection(new_seg.target());
                const Kernel::Segment_2 pseg(pa, pb);

                auto variant = CGAL::intersection(pseg, seg);
                if (variant) {

                    if (boost::get<Kernel::Segment_2>(&*variant) != nullptr) {
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

auto SkeletonRadial::polylineLength(const std::vector<Kernel::Point_2>& result) const -> double {
    double length = 0;
    for (size_t i = 1; i < result.size(); ++i) {

        length += sqrt(CGAL::to_double(CGAL::squared_distance(result.at(i - 1), result.at(i))));
    }
    return length;
}

auto SkeletonRadial::fillCorner(
    const basic::Arrangement_2Node& arr3,
    const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache) -> void {
    std::vector<Kernel::Point_2> pointVect;
    for (const auto& line : _radial_list.lines()) {
        for (const auto& p : line.points()) {
            pointVect.emplace_back(p);
        }
    }
    CGAL::Point_set_2<Kernel> pt_set(pointVect.begin(), pointVect.end());
    double sep = _config.sep; //* M_SQRT1_2;

    // I iterate through arr3 instead of faceCache, in order to be predicatable
    for (const basic::FaceNode& faceNode :
         RangeHelper::make(arr3.faces_begin(), arr3.faces_end())) {
        auto ite = faceCache.find(&faceNode);
        if (ite != faceCache.end()) {
            const FaceHelper& faceHelper = ite->second;
            if (faceHelper._type == FaceHelper::Type::Corner) {
                for (const basic::HalfedgeNode& he : RangeHelper::make(faceNode.outer_ccb())) {

                    if (faceHelper._o != he.source()->point() and
                        faceHelper._o != he.target()->point()) {

                        iterateCorner(he, sep, pt_set, faceCache);
                    }
                }
            }
        }
    }
}

auto SkeletonRadial::fillRadialList(std::vector<Kernel::Point_2>& result,
                                    CGAL::Point_set_2<Kernel>& pt_set) -> void {
    if (result.size() < 2 || polylineLength(result) < (_config.min_length_polyline)) {
        // do nothing
    } else {
        pt_set.insert(result.front());
        pt_set.insert(result.back());
        _radial_list.lines().emplace_back(0, result);
    }
}

auto SkeletonRadial::startTraverseCorner(
    const Kernel::Point_2& test, const double& sep, const basic::HalfedgeNode& hedge2,
    const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache,
    CGAL::Point_set_2<Kernel>& pt_set) -> std::vector<Kernel::Point_2> {

    std::vector<Kernel::Point_2> result;
    auto vertex_handle = pt_set.nearest_neighbor(test);
    CGAL::Comparison_result compare = CGAL::LARGER;
    if (vertex_handle == nullptr) {
        compare = CGAL::LARGER;
    } else {
        compare = CGAL::compare_squared_distance(test, vertex_handle->point(), sep * sep);
    }
    if (compare == CGAL::LARGER) {
        Incidence incid(test, hedge2);
        result.emplace_back(test);
        int32_t limit = _config.max_polyline_element;
        while (!incid.isEmpty && limit > 0) {

            incid = traverseFace(incid, faceCache, result);
            --limit;
        }
        if (limit == 0) {
            std::cout << "limit exceeded" << std::endl;
        }
    }
    return result;
}

auto SkeletonRadial::startTraverseEdge(
    const Kernel::Point_2& test, const double& sep, const basic::HalfedgeNode& hedge2,
    const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache,
    CGAL::Point_set_2<Kernel>& pt_set) -> void {
    auto vertex_handle = pt_set.nearest_neighbor(test);
    CGAL::Comparison_result compare = CGAL::LARGER;
    if (vertex_handle == nullptr) {
        compare = CGAL::LARGER;
    } else {
        compare = CGAL::compare_squared_distance(test, vertex_handle->point(), sep * sep);
    }
    if (compare == CGAL::LARGER) {
        Incidence incid(test, hedge2);
        std::vector<Kernel::Point_2> result;
        result.emplace_back(test);
        int32_t limit = _config.max_polyline_element;
        while (!incid.isEmpty && limit > 0) {
            incid = traverseFace(incid, faceCache, result);
            --limit;
        }
        if (limit == 0) {
            std::cout << "limit exceeded" << std::endl;
        }
        fillRadialList(result, pt_set);
    }
}

auto SkeletonRadial::iterateEdge(
    const basic::HalfedgeNode& hedge, const double& sep, CGAL::Point_set_2<Kernel>& pt_set,
    const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache) -> void {
    const Kernel::Point_2& origin = hedge.source()->point();
    const Kernel::Segment_2& curve = hedge.curve();

    double weight = (_config.sep_subdivision / sep) * sqrt(CGAL::to_double(curve.squared_length()));

    double sign = 1;
    for (double i = _config.displacement; i < weight; ++i) {
        const Kernel::Point_2 test = CGAL::barycenter(
            origin, sign * (i / (2. * weight)) + (1. / 2.), hedge.target()->point());
        sign *= -1;
        startTraverseEdge(test, sep, hedge, faceCache, pt_set);
    }
}

auto SkeletonRadial::iterateCorner(
    const basic::HalfedgeNode& hedge, const double& sep, CGAL::Point_set_2<Kernel>& pt_set,
    const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache) -> void {
    const Kernel::Point_2& origin = hedge.source()->point();
    const Kernel::Segment_2& curve = hedge.curve();

    double weight = (_config.sep_subdivision / sep) * sqrt(CGAL::to_double(curve.squared_length()));

    double sign = 1;
    for (double i = _config.displacement; i < weight; ++i) {
        const Kernel::Point_2 test = CGAL::barycenter(
            origin, sign * (i / (2. * weight)) + (1. / 2.), hedge.target()->point());
        sign *= -1;
        std::vector<Kernel::Point_2> result =
            startTraverseCorner(test, sep, hedge, faceCache, pt_set);
        std::vector<Kernel::Point_2> result2 =
            startTraverseCorner(test, sep, *hedge.twin(), faceCache, pt_set);
        std::reverse(result.begin(), result.end());
        if (!result.empty()) {
            result.resize(result.size() - 1);
        }
        result.insert(result.end(), result2.begin(), result2.end());

        if (result.size() < 2 || polylineLength(result) < (_config.min_length_polyline)) {
            // do nothing
        } else {
            pt_set.insert(test);

            _radial_list.lines().emplace_back(0, result);
        }
    }
}

void SkeletonRadial::cropLine(const Kernel::Vector_2 vect, const Kernel::Line_2& line,
                              const Kernel::Iso_rectangle_2 bbox,
                              std::vector<Kernel::Segment_2>& result2) const {
    auto obj = CGAL::intersection(line, bbox);
    if (obj) {
        const Kernel::Segment_2* s = boost::get<Kernel::Segment_2>(&*obj);

        if (s != nullptr) {
            Kernel::Segment_2 ss = Kernel::Segment_2(s->source() + vect, s->target() + vect);
            result2.emplace_back(ss);
        }
    }
}

auto SkeletonRadial::polygonContour(
    const basic::Arrangement_2Node& arr3,
    const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache,
    CGAL::Point_set_2<Kernel>& pt_set) -> void {

    std::vector<const basic::HalfedgeNode*> heList;
    heList.reserve(arr3.number_of_edges());
    for (const basic::HalfedgeNode& he : RangeHelper::make(arr3.edges_begin(), arr3.edges_end())) {
        heList.emplace_back(&he);
    }
    _random.shuffle(heList);

    for (const basic::HalfedgeNode* he : heList) {
        // we meet polygon edge
        if (basic::edgeHasPolygonId(*he, +1)) {
            // we must test if this is a hole
            if (basic::edgeHasPolygonId(*he->next(), +1)) {
                iterateEdge(*he->twin(), _config.sep, pt_set, faceCache);
            } else {
                iterateEdge(*he, _config.sep, pt_set, faceCache);
            }
        }
    }
}
auto SkeletonRadial::radialList() const -> std::vector<Kernel::Segment_2> {

    if (_radial_list.lines().empty()) {
        return {};
    }
    return _radial_list.giveSpace(laby::GiveSpaceConfig{_config.sep, 3, _config.sep * 2.})
        .getSegments();
}

auto SkeletonRadial::getPointIntersect(const CGAL::Polygon_with_holes_2<Kernel>& poly_hole) const
    -> CGAL::Point_set_2<Kernel> {
    typedef std::vector<Kernel::Segment_2> Segment_2Vect;
    typedef Segment_2Vect::const_iterator Iterator;
    typedef CGAL::Box_intersection_d::Box_with_handle_d<double, 2, Iterator> Box;

    std::vector<Box> radial_boxes;

    std::vector<Kernel::Segment_2> list = radialList();
    for (auto i = list.begin(); i != list.end(); ++i) {
        radial_boxes.push_back(Box(i->bbox(), i));
    }
    CGAL::Point_set_2<Kernel> pt_set;

    std::vector<Kernel::Segment_2> poly_seg;
    const auto& outer = poly_hole.outer_boundary();
    for (const CGAL::Segment_2<Kernel> seg :
         RangeHelper::make(outer.edges_begin(), outer.edges_end())) {
        poly_seg.emplace_back(seg);
    }
    for (const CGAL::Polygon_2<Kernel>& hole :
         RangeHelper::make(poly_hole.holes_begin(), poly_hole.holes_end())) {
        for (const CGAL::Segment_2<Kernel> seg :
             RangeHelper::make(hole.edges_begin(), hole.edges_end())) {
            poly_seg.emplace_back(seg);
        }
    }
    std::vector<Box> poly_boxes;
    for (auto i = poly_seg.begin(); i != poly_seg.end(); ++i) {
        poly_boxes.push_back(Box(i->bbox(), i));
    }
    CGAL::box_intersection_d(
        radial_boxes.begin(), radial_boxes.end(), poly_boxes.end(), poly_boxes.end(), //
        [&](const Box& a, const Box& b) {
            std::cout << "box " << a.id() << " intersects box " << b.id() << std::endl;

            const Kernel::Segment_2& radial = *a.handle();
            const Kernel::Segment_2& poly = *b.handle();

            auto variant2 = CGAL::intersection(radial, poly);
            if (variant2) {
                if (const Kernel::Point_2* p2 = boost::get<Kernel::Point_2>(&*variant2)) {
                    pt_set.insert(*p2);
                }
            }
        });
    return pt_set;
}

auto SkeletonRadial::createRadial(const basic::Arrangement_2Node& arr3,
                                  const CGAL::Polygon_with_holes_2<Kernel>& poly_hole) -> void {

    std::unordered_map<const basic::FaceNode*, FaceHelper> faceCache;

    for (const basic::HalfedgeNode& he : RangeHelper::make(arr3.edges_begin(), arr3.edges_end())) {
        // we meet polygon edge
        if (basic::edgeHasPolygonId(he, +1)) {
            // we must test if this is a hole
            if (basic::edgeHasPolygonId(*he.next(), +1)) {
                registerFace(*he.twin(), faceCache);
            } else {
                registerFace(he, faceCache);
            }
        }
    }
    CGAL::Point_set_2<Kernel> pt_set = getPointIntersect(poly_hole);
    polygonContour(arr3, faceCache, pt_set);
    fillCorner(arr3, faceCache);
}

} /* namespace laby */
