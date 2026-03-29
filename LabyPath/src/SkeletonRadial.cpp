/*
 * SkeletonRadial.cpp
 *
 *  Created on: Jul 9, 2018
 *      Author: florian
 */

#include "SkeletonRadial.h"

#include <CGAL/Box_intersection_d/Box_with_handle_d.h>
#include <CGAL/Distance_2/Point_2_Point_2.h>
#include <CGAL/Intersections_2/Line_2_Segment_2.h>
#include <CGAL/Kernel/global_functions_2.h>
#include <CGAL/box_intersection_d.h>
#include <CGAL/enum.h>
#include <CGAL/number_utils.h>

#include "basic/AugmentedPolygonSet.h"
#include <GeomData.h>
#include <algorithm>
#include <basic/RangeHelper.h>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <variant>
#include <vector>

namespace laby {

auto SkeletonRadial::registerFace(
    const basic::HalfedgeNode& halfedge,
    std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache) const -> void {
    {
        FaceHelper& faceHelper =
            faceCache.try_emplace(&*halfedge.face(), FaceHelper()).first->second;
        faceHelper._perp =
            (halfedge.curve().target() - halfedge.curve().source()).perpendicular(CGAL::LEFT_TURN);
        if (faceHelper._type != FaceHelper::Type::Unknown) {
            std::cout << "face lateral already exists with " << static_cast<int>(faceHelper._type)
                      << '\n';
        }
        faceHelper._type = FaceHelper::Type::Lateral;
    }
    const auto& originVertex = *halfedge.source();
    if (originVertex.degree() < 4) {
        return;
    }
    basic::Arrangement_2Node::Halfedge_around_vertex_const_circulator circ =
        originVertex.incident_halfedges();

    while (!basic::edgeHasPolygonId(*circ, +1)) {
        ++circ;
    }

    while (basic::edgeHasPolygonId(*circ, +1)) {
        ++circ;
    }

    const basic::HalfedgeNode& heSecond = *circ;
    const basic::HalfedgeNode& heFirst = *heSecond.next();

    if (&*heSecond.face() != &*heFirst.face()) {
        std::cout << "problem of face" << '\n';
    }
    FaceHelper& faceHelper = faceCache.try_emplace(&*heSecond.face(), FaceHelper()).first->second;

    if (faceHelper._type != FaceHelper::Type::Unknown) {
        std::cout << "face corner already exists with " << static_cast<int>(faceHelper._type)
                  << '\n';
    }

    faceHelper._type = FaceHelper::Type::Corner;
    faceHelper._o = originVertex.point();
}

auto SkeletonRadial::traverseCorner(const Incidence& incidence, const FaceHelper& faceHelper,
                                    std::vector<Kernel::Point_2>& result) const -> Incidence {

    Kernel::Line_2 const line = Kernel::Line_2(incidence.testPoint(), faceHelper._o);

    for (const basic::HalfedgeNode& halfedge : RangeHelper::make(incidence.halfedge()->ccb())) {
        auto variant2 = CGAL::intersection(line, Kernel::Segment_2(halfedge.curve()));
        if (variant2) {
            if (const Kernel::Point_2* pointIntersection =
                    std::get_if<Kernel::Point_2>(&*variant2)) {
                if (CGAL::squared_distance(*pointIntersection, faceHelper._o) > 0.0 &&
                    CGAL::squared_distance(*pointIntersection, incidence.testPoint()) > 0.0) {
                    if (filterNewVertex(result, *pointIntersection)) {
                        result.emplace_back(*pointIntersection);
                        return {*pointIntersection, *halfedge.twin()};
                    }
                }
            } else {
                std::cout << " variant 2 is not a point ";
            }
        }
    }

    result.emplace_back(faceHelper._o);
    return {};
}

auto SkeletonRadial::traverseRay(const basic::HalfedgeNode& halfedge,
                                 const Kernel::Point_2& testPoint,
                                 const Kernel::Vector_2& direction,
                                 std::vector<Kernel::Point_2>& result) const -> Incidence {

    Kernel::Line_2 const line(testPoint, direction);
    for (const basic::HalfedgeNode& candidateHalfedge : RangeHelper::make(halfedge.next()->ccb())) {
        auto variant = CGAL::intersection(line, Kernel::Segment_2(candidateHalfedge.curve()));
        if (variant) {
            if (const Kernel::Point_2* pointIntersection =
                    std::get_if<Kernel::Point_2>(&*variant)) {
                if (*pointIntersection != testPoint) {
                    if (filterNewVertex(result, *pointIntersection)) {
                        result.emplace_back(*pointIntersection);
                        return {*pointIntersection, *candidateHalfedge.twin()};
                    }
                    return {};
                }
            } else {
                std::cout << "variant is not a point" << '\n';
            }
        }
    }
    std::cout << "test ray is " << testPoint << '\n';
    for (const basic::HalfedgeNode& candidateHalfedge : RangeHelper::make(halfedge.next()->ccb())) {
        std::cout << candidateHalfedge.curve() << '\n';
    }
    std::cout << "no incidence traverse_ray!" << '\n';
    return {};
}

auto SkeletonRadial::traverseFace(
    const Incidence& incidence,
    const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache, //
    std::vector<Kernel::Point_2>& result) const -> SkeletonRadial::Incidence {
    auto faceIterator = faceCache.find(&*incidence.halfedge()->face());

    if (faceIterator == faceCache.end()) {
        return {};
    }
    const FaceHelper& faceHelper = faceIterator->second;
    switch (faceHelper._type) {
    case FaceHelper::Type::Lateral: {
        // to be calculated at the beginning, at the creation of FaceHelper

        return traverseRay(*incidence.halfedge(), incidence.testPoint(), faceHelper._perp, result);
    }
    case FaceHelper::Type::Corner: {
        Kernel::Line_2 const line;
        static_cast<void>(line);

        return traverseCorner(incidence, faceHelper, result);
    }
    case FaceHelper::Type::Unknown: {
        std::cout << "Unknow faced " << '\n';
        break;
    }
    }
    return {};
}

auto SkeletonRadial::filterNewVertex(const std::vector<Kernel::Point_2>& result,
                                     const Kernel::Point_2& vertex) const -> bool {
    if (result.size() < 2) {
        return true;
    }
    const Kernel::Segment_2 newSeg(result.at(result.size() - 1), vertex);
    for (std::size_t i = result.size() - 1; i > 0; --i) {
        const Kernel::Segment_2 seg(result.at(i - 1), result.at(i));

        double const cosTheta =
            CGAL::to_double(CGAL::scalar_product(newSeg.to_vector(), seg.to_vector())) / //
            (sqrt(CGAL::to_double(newSeg.squared_length())) *
             sqrt(CGAL::to_double(seg.squared_length())));

        if (cosTheta * cosTheta > _config.cosTheta * _config.cosTheta) {
            if (CGAL::squared_distance(newSeg, seg) <
                _config.filtered_distance * _config.filtered_distance) {
                const Kernel::Point_2 projectedStart =
                    seg.supporting_line().projection(newSeg.source());
                const Kernel::Point_2 projectedEnd =
                    seg.supporting_line().projection(newSeg.target());
                const Kernel::Segment_2 projectedSegment(projectedStart, projectedEnd);

                auto variant = CGAL::intersection(projectedSegment, seg);
                if (variant) {

                    if (std::get_if<Kernel::Segment_2>(&*variant) != nullptr) {
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
    for (std::size_t pointIndex = 1; pointIndex < result.size(); ++pointIndex) {

        length += sqrt(CGAL::to_double(
            CGAL::squared_distance(result.at(pointIndex - 1), result.at(pointIndex))));
    }
    return length;
}

auto SkeletonRadial::fillCorner(
    const basic::Arrangement_2Node& arr3,
    const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache) -> void {
    std::vector<Kernel::Point_2> pointVect;
    for (const auto& line : _radial_list.lines()) {
        for (const auto& point : line.points()) {
            pointVect.emplace_back(point);
        }
    }
    CGAL::Point_set_2<Kernel> ptSet(pointVect.begin(), pointVect.end());
    double const sep = _config.sep; //* M_SQRT1_2;

    // I iterate through arr3 instead of faceCache, in order to be predicatable
    for (const basic::FaceNode& faceNode :
         RangeHelper::make(arr3.faces_begin(), arr3.faces_end())) {
        auto faceIterator = faceCache.find(&faceNode);
        if (faceIterator != faceCache.end()) {
            const FaceHelper& faceHelper = faceIterator->second;
            if (faceHelper._type == FaceHelper::Type::Corner) {
                for (const basic::HalfedgeNode& halfedge :
                     RangeHelper::make(faceNode.outer_ccb())) {

                    if (faceHelper._o != halfedge.source()->point() &&
                        faceHelper._o != halfedge.target()->point()) {

                        iterateCorner(halfedge, sep, ptSet, faceCache);
                    }
                }
            }
        }
    }
}

auto SkeletonRadial::fillRadialList(std::vector<Kernel::Point_2>& result,
                                    CGAL::Point_set_2<Kernel>& pointSet) -> void {
    if (result.size() < 2 || polylineLength(result) < (_config.min_length_polyline)) {
        // do nothing
    } else {
        pointSet.insert(result.front());
        pointSet.insert(result.back());
        _radial_list.lines().emplace_back(0, result);
    }
}

auto SkeletonRadial::startTraverseCorner(
    const Kernel::Point_2& test, const double& sep, const basic::HalfedgeNode& halfedge,
    const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache,
    CGAL::Point_set_2<Kernel>& pointSet) -> std::vector<Kernel::Point_2> {

    std::vector<Kernel::Point_2> result;
    auto vertexHandle = pointSet.nearest_neighbor(test);
    CGAL::Comparison_result compare = CGAL::LARGER;
    if (vertexHandle == nullptr) {
        compare = CGAL::LARGER;
    } else {
        compare = CGAL::compare_squared_distance(test, vertexHandle->point(), sep * sep);
    }
    if (compare == CGAL::LARGER) {
        Incidence incidence(test, halfedge);
        result.emplace_back(test);
        int32_t limit = _config.max_polyline_element;
        while (!incidence.isEmpty() && limit > 0) {

            incidence = traverseFace(incidence, faceCache, result);
            --limit;
        }
        if (limit == 0) {
            std::cout << "limit exceeded" << std::endl;
        }
    }
    return result;
}

auto SkeletonRadial::startTraverseEdge(
    const Kernel::Point_2& test, const double& sep, const basic::HalfedgeNode& halfedge,
    const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache,
    CGAL::Point_set_2<Kernel>& pointSet) -> void {
    auto vertexHandle = pointSet.nearest_neighbor(test);
    CGAL::Comparison_result compare = CGAL::LARGER;
    if (vertexHandle == nullptr) {
        compare = CGAL::LARGER;
    } else {
        compare = CGAL::compare_squared_distance(test, vertexHandle->point(), sep * sep);
    }
    if (compare == CGAL::LARGER) {
        Incidence incidence(test, halfedge);
        std::vector<Kernel::Point_2> result;
        result.emplace_back(test);
        int32_t limit = _config.max_polyline_element;
        while (!incidence.isEmpty() && limit > 0) {
            incidence = traverseFace(incidence, faceCache, result);
            --limit;
        }
        if (limit == 0) {
            std::cout << "limit exceeded" << std::endl;
        }
        fillRadialList(result, pointSet);
    }
}

auto SkeletonRadial::iterateEdge(
    const basic::HalfedgeNode& halfedge, const double& sep, CGAL::Point_set_2<Kernel>& pointSet,
    const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache) -> void {
    const Kernel::Point_2& origin = halfedge.source()->point();
    const Kernel::Segment_2& curve = halfedge.curve();

    const double weight =
        (_config.sep_subdivision / sep) * sqrt(CGAL::to_double(curve.squared_length()));
    const int iterationCount = static_cast<int>(std::ceil(weight - _config.displacement));

    double sign = 1.0;
    for (int iteration = 0; iteration < iterationCount; ++iteration) {
        const double offset = _config.displacement + static_cast<double>(iteration);
        const Kernel::Point_2 test = CGAL::barycenter(
            origin, sign * (offset / (2.0 * weight)) + 0.5, halfedge.target()->point());
        sign *= -1;
        startTraverseEdge(test, sep, halfedge, faceCache, pointSet);
    }
}

auto SkeletonRadial::iterateCorner(
    const basic::HalfedgeNode& halfedge, const double& sep, CGAL::Point_set_2<Kernel>& pointSet,
    const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache) -> void {
    const Kernel::Point_2& origin = halfedge.source()->point();
    const Kernel::Segment_2& curve = halfedge.curve();

    const double weight =
        (_config.sep_subdivision / sep) * sqrt(CGAL::to_double(curve.squared_length()));
    const int iterationCount = static_cast<int>(std::ceil(weight - _config.displacement));

    double sign = 1.0;
    for (int iteration = 0; iteration < iterationCount; ++iteration) {
        const double offset = _config.displacement + static_cast<double>(iteration);
        const Kernel::Point_2 test = CGAL::barycenter(
            origin, sign * (offset / (2.0 * weight)) + 0.5, halfedge.target()->point());
        sign *= -1;
        std::vector<Kernel::Point_2> result =
            startTraverseCorner(test, sep, halfedge, faceCache, pointSet);
        std::vector<Kernel::Point_2> result2 =
            startTraverseCorner(test, sep, *halfedge.twin(), faceCache, pointSet);
        std::reverse(result.begin(), result.end());
        if (!result.empty()) {
            result.resize(result.size() - 1);
        }
        result.insert(result.end(), result2.begin(), result2.end());

        if (result.size() < 2 || polylineLength(result) < (_config.min_length_polyline)) {
            // do nothing
        } else {
            pointSet.insert(test);

            _radial_list.lines().emplace_back(0, result);
        }
    }
}

auto SkeletonRadial::cropLine(const Kernel::Vector_2 vector, const Kernel::Line_2& line,
                              const Kernel::Iso_rectangle_2 boundingBox,
                              std::vector<Kernel::Segment_2>& resultSegments) const -> void {
    auto obj = CGAL::intersection(line, boundingBox);
    if (obj) {
        const Kernel::Segment_2* croppedSegment = std::get_if<Kernel::Segment_2>(&*obj);

        if (croppedSegment != nullptr) {
            Kernel::Segment_2 shiftedSegment(croppedSegment->source() + vector,
                                             croppedSegment->target() + vector);
            resultSegments.emplace_back(shiftedSegment);
        }
    }
}

auto SkeletonRadial::polygonContour(
    const basic::Arrangement_2Node& arrangement,
    const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache,
    CGAL::Point_set_2<Kernel>& pointSet) -> void {

    std::vector<const basic::HalfedgeNode*> halfedgeList;
    halfedgeList.reserve(arrangement.number_of_edges());
    for (const basic::HalfedgeNode& halfedge :
         RangeHelper::make(arrangement.edges_begin(), arrangement.edges_end())) {
        halfedgeList.emplace_back(&halfedge);
    }
    _random.shuffle(halfedgeList);

    for (const basic::HalfedgeNode* halfedge : halfedgeList) {
        // we meet polygon edge
        if (basic::edgeHasPolygonId(*halfedge, +1)) {
            // we must test if this is a hole
            if (basic::edgeHasPolygonId(*halfedge->next(), +1)) {
                iterateEdge(*halfedge->twin(), _config.sep, pointSet, faceCache);
            } else {
                iterateEdge(*halfedge, _config.sep, pointSet, faceCache);
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

auto SkeletonRadial::getPointIntersect(
    const CGAL::Polygon_with_holes_2<Kernel>& polygonWithHoles) const -> CGAL::Point_set_2<Kernel> {
    typedef std::vector<Kernel::Segment_2> Segment_2Vect;
    typedef Segment_2Vect::const_iterator Iterator;
    typedef CGAL::Box_intersection_d::Box_with_handle_d<double, 2, Iterator> Box;

    std::vector<Box> radialBoxes;

    std::vector<Kernel::Segment_2> radialSegments = radialList();
    for (auto radialIt = radialSegments.begin(); radialIt != radialSegments.end(); ++radialIt) {
        radialBoxes.push_back(Box(radialIt->bbox(), radialIt));
    }
    CGAL::Point_set_2<Kernel> ptSet;

    std::vector<Kernel::Segment_2> polygonSegments;
    const auto& outerBoundary = polygonWithHoles.outer_boundary();
    for (const CGAL::Segment_2<Kernel> seg :
         RangeHelper::make(outerBoundary.edges_begin(), outerBoundary.edges_end())) {
        polygonSegments.emplace_back(seg);
    }
    for (const CGAL::Polygon_2<Kernel>& hole :
         RangeHelper::make(polygonWithHoles.holes_begin(), polygonWithHoles.holes_end())) {
        for (const CGAL::Segment_2<Kernel> seg :
             RangeHelper::make(hole.edges_begin(), hole.edges_end())) {
            polygonSegments.emplace_back(seg);
        }
    }
    std::vector<Box> polyBoxes;
    for (auto polygonIt = polygonSegments.begin(); polygonIt != polygonSegments.end();
         ++polygonIt) {
        polyBoxes.push_back(Box(polygonIt->bbox(), polygonIt));
    }
    CGAL::box_intersection_d(
        radialBoxes.begin(), radialBoxes.end(), polyBoxes.begin(), polyBoxes.end(),
        [&](const Box& a, const Box& b) {
            std::cout << "box " << a.id() << " intersects box " << b.id() << std::endl;

            const Kernel::Segment_2& radial = *a.handle();
            const Kernel::Segment_2& poly = *b.handle();

            auto variant2 = CGAL::intersection(radial, poly);
            if (variant2) {
                if (const Kernel::Point_2* p2 = std::get_if<Kernel::Point_2>(&*variant2)) {
                    ptSet.insert(*p2);
                }
            }
        });
    return ptSet;
}

auto SkeletonRadial::createRadial(const basic::Arrangement_2Node& arrangement,
                                  const CGAL::Polygon_with_holes_2<Kernel>& polygonWithHoles)
    -> void {

    std::unordered_map<const basic::FaceNode*, FaceHelper> faceCache;

    for (const basic::HalfedgeNode& halfedge :
         RangeHelper::make(arrangement.edges_begin(), arrangement.edges_end())) {
        // we meet polygon edge
        if (basic::edgeHasPolygonId(halfedge, +1)) {
            // we must test if this is a hole
            if (basic::edgeHasPolygonId(*halfedge.next(), +1)) {
                registerFace(*halfedge.twin(), faceCache);
            } else {
                registerFace(halfedge, faceCache);
            }
        }
    }
    CGAL::Point_set_2<Kernel> ptSet = getPointIntersect(polygonWithHoles);
    polygonContour(arrangement, faceCache, ptSet);
    fillCorner(arrangement, faceCache);
}

} /* namespace laby */
