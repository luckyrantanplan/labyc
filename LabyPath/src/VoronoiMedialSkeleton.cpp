/*
 * VoronoiSeg.cpp
 *
 *  Created on: Oct 25, 2017
 *      Author: florian
 */

// standard includes
#include "VoronoiMedialSkeleton.h"

#include <CGAL/Arrangement_2/Arrangement_on_surface_2_global.h>
#include <CGAL/Arrangement_on_surface_2.h>
#include <CGAL/Bbox_2.h>
#include <CGAL/Intersections_2/Line_2_Line_2.h>
#include <CGAL/Kernel/global_functions_2.h>
#include <CGAL/Object.h>
#include <CGAL/Parabola_segment_2.h>
#include <CGAL/Point_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/Segment_2.h>
#include <CGAL/enum.h>
#include <GeomData.h>
#include <basic/AugmentedPolygonSet.h>
#include <basic/RangeHelper.h>
#include <cstddef>
#include <iostream>
#include <utility>
#include <variant>
#include <vector>

#include "Polyline.h"
#include "Ribbon.h"
#include "agg/agg_curves.h"
#include "basic/KernelConverter.h"

namespace laby {
class Ribbon;
} /* namespace laby */

namespace laby {

auto CroppedVoronoiFromDelaunay::CustomParabola::getTangent(const Point_2& point)
    -> Kernel_sqrt::Line_2 {
    if (point == o) {
        return Line_2(o, l.direction()); // line qui passe par o et qui a pour direction l
    }
    Line_2 const projectedNormal(l.projection(point), point);
    Line_2 const focusLine(c, point);
    return CGAL::bisector(projectedNormal, focusLine);
}

auto CroppedVoronoiFromDelaunay::CustomParabola::computeBezier()
    -> CroppedVoronoiFromDelaunay::CustomParabola::Point_2 {

    if (p1 == p2) {
        return p1;
    }

    Kernel_sqrt::Line_2 const tangent1 = getTangent(p1);
    Kernel_sqrt::Line_2 const tangent2 = getTangent(p2);
    auto result4 = CGAL::intersection(tangent1, tangent2);

    if (!result4) {
        std::cout << " no intersection " << '\n';
        return CGAL::midpoint(p1, p2);
    }
    if (const Kernel_sqrt::Point_2* bezierControlPoint =
            std::get_if<Kernel_sqrt::Point_2>(&*result4)) {
        return *bezierControlPoint;
    }
    std::cout << "no bezier_control_point" << '\n';
    ;
    return CGAL::midpoint(p1, p2);
}

void CroppedVoronoiFromDelaunay::drawDual(const SDG2& sdg) {
    To_sqrt_kernel const toSqrtKernel;

    for (const auto& edge : RangeHelper::make(sdg.finite_edges_begin(), sdg.finite_edges_end())) {

        Line_2 line;
        Segment_2 segment;
        Ray_2 ray;
        CGAL::Parabola_segment_2<Gt> parabolaSegment;
        CGAL::Object const primalObject = sdg.primal(edge);
        if (CGAL::assign(line, primalObject)) {
            //        cropAndExtractSegment(l);
        }

        if (CGAL::assign(segment, primalObject)) {
            cropAndExtractSegment(segment);
        }
        if (CGAL::assign(ray, primalObject)) {
            //        cropAndExtractSegment(r);
        }
        if (CGAL::assign(parabolaSegment, primalObject)) {
            CustomParabola customParabola(parabolaSegment);
            auto controlPoint = customParabola.computeBezier();

            auto startPoint = customParabola.getP1();
            auto endPoint = customParabola.getP2();
            agg::Curve3 curve(CGAL::to_double(startPoint.x()), CGAL::to_double(startPoint.y()),
                              CGAL::to_double(controlPoint.x()), CGAL::to_double(controlPoint.y()),
                              CGAL::to_double(endPoint.x()), CGAL::to_double(endPoint.y()));

            Kernel_sqrt::Point_2 previousPoint = startPoint;
            const auto& curvePoints = curve.getPoints();
            for (std::size_t pointIndex = 1; pointIndex < curvePoints.size() - 1; ++pointIndex) {
                Kernel_sqrt::Point_2 curvePoint = toSqrtKernel(curvePoints.at(pointIndex));
                cropAndExtractSegment(Segment_2(previousPoint, curvePoint));
                previousPoint = curvePoint;
            }
            cropAndExtractSegment(Segment_2(previousPoint, endPoint));
        }
    }
}

void VoronoiMedialSkeleton::addSimplePolygon(
    const CGAL::Polygon_2<Kernel>& outerBoundary, std::vector<Gt::Point_2>& points,
    std::vector<std::pair<std::size_t, std::size_t>>& indices) {
    SDG2::Site_2 site;
    std::size_t pointIndex = points.size();

    std::size_t const savedIndex = points.size();
    To_sqrt_kernel const toSqrtKernel;
    CGAL::Polygon_2<Kernel>::Edge_const_iterator const ite = outerBoundary.edges_begin();
    site = SDG2::Site_2::construct_site_2(toSqrtKernel(ite->source()), toSqrtKernel(ite->target()));

    points.push_back(site.source_of_supporting_site());
    for (const CGAL::Segment_2<Kernel> seg :
         RangeHelper::make(outerBoundary.edges_begin(), outerBoundary.edges_end())) {
        site =
            SDG2::Site_2::construct_site_2(toSqrtKernel(seg.source()), toSqrtKernel(seg.target()));
        indices.emplace_back(pointIndex, pointIndex + 1);
        points.push_back(site.target_of_supporting_site());
        ++pointIndex;
    }
    indices.emplace_back(pointIndex, savedIndex);
}

void VoronoiMedialSkeleton::addPolyline(const Polyline& polyline, std::vector<Gt::Point_2>& points,
                                        std::vector<std::pair<std::size_t, std::size_t>>& indices) {
    const std::vector<Point_2>& polylinePoints = polyline.points();

    if (polylinePoints.size() < 2) {
        return;
    }
    SDG2::Site_2 site;
    std::size_t pointIndex = points.size();

    std::size_t const savedIndex = points.size();
    To_sqrt_kernel const toSqrtKernel;

    site = SDG2::Site_2::construct_site_2(toSqrtKernel(polylinePoints.at(0)),
                                          toSqrtKernel(polylinePoints.at(1)));
    points.push_back(site.source_of_supporting_site());

    for (std::size_t sourceIndex = 1; sourceIndex < polylinePoints.size(); ++sourceIndex) {
        const Point_2& source = polylinePoints.at(sourceIndex - 1);
        const Point_2& target = polylinePoints.at(sourceIndex);
        site = SDG2::Site_2::construct_site_2(toSqrtKernel(source), toSqrtKernel(target));
        indices.emplace_back(pointIndex, pointIndex + 1);
        points.push_back(site.target_of_supporting_site());
        ++pointIndex;
    }
    if (polyline.isClosed()) {
        indices.emplace_back(pointIndex, savedIndex);
    }
}

void VoronoiMedialSkeleton::addPolygon(const CGAL::Polygon_with_holes_2<Kernel>& polygon,
                                       SDG2& sdg) {

    std::vector<Gt::Point_2> points;
    // segments of the polygon as a pair of point indices
    std::vector<std::pair<std::size_t, std::size_t>> indices;

    addSimplePolygon(polygon.outer_boundary(), points, indices);

    for (const auto& hole : RangeHelper::make(polygon.holes_begin(), polygon.holes_end())) {
        addSimplePolygon(hole, points, indices);
    }
    sdg.insert_segments(points.begin(), points.end(), indices.begin(), indices.end());
}

VoronoiMedialSkeleton::VoronoiMedialSkeleton(const std::vector<Segment_info_2>& seg_list,
                                             const CGAL::Bbox_2& bbox) {
    SDG2 sdg;
    std::vector<SDG2::Site_2> sites;
    // segments of the polygon as a pair of point indices
    std::vector<Kernel::Point_2> pointsK;
    To_sqrt_kernel const toSqrtKernel;

    for (const Segment_info_2& seg : seg_list) {
        sites.emplace_back(
            SDG2::Site_2::construct_site_2(toSqrtKernel(seg.source()), toSqrtKernel(seg.target())));
        pointsK.emplace_back(seg.source());
        pointsK.emplace_back(seg.target());
    }

    sdg.insert_segments(sites.begin(), sites.end());
    std::cout << "start cropping" << std::endl;

    Kernel_sqrt::Iso_rectangle_2 rect(bbox.xmin() - 1, bbox.ymin() - 1, bbox.xmax() + 1,
                                      bbox.ymax() + 1);

    auto vor = CroppedVoronoiFromDelaunay(rect);
    std::cout << "CroppedVoronoiFromDelaunay " << std::endl;

    vor.drawDual(sdg);
    std::cout << " sdg.drawDual " << std::endl;

    // we use a point set, in order to get back to original Kernel in an exact way
    CGAL::Point_set_2<Kernel> pointSet;

    pointSet.insert(pointsK.begin(), pointsK.end());

    From_sqrt_kernel fromSqrtKernel;
    for (const auto& segment : vor.m_cropped_vd) {
        auto kernelSeg = fromSqrtKernel(segment);
        const Point_2 sourcePoint = getCachePoint(kernelSeg.source(), pointSet);
        const Point_2 targetPoint = getCachePoint(kernelSeg.target(), pointSet);
        CGAL::Comparison_result compare =
            CGAL::compare_squared_distance(sourcePoint, targetPoint, 0.);

        if (compare == CGAL::LARGER) {
            _result.emplace_back(Segment_2(sourcePoint, targetPoint));
        }
    }
}

VoronoiMedialSkeleton::VoronoiMedialSkeleton(const Ribbon& ribbon, const CGAL::Bbox_2& bbox) {
    std::cout << "add ribbon   " << std::endl;

    SDG2 sdg;
    std::vector<Gt::Point_2> points;
    std::vector<Kernel::Point_2> pointsK;
    // segments of the polygon as a pair of point indices
    std::vector<std::pair<std::size_t, std::size_t>> indices;
    for (const Polyline& polyline : ribbon.lines()) {
        addPolyline(polyline, points, indices);
        pointsK.insert(pointsK.end(), polyline.points().begin(), polyline.points().end());
    }
    sdg.insert_segments(points.begin(), points.end(), indices.begin(), indices.end());
    std::cout << "start cropping" << std::endl;

    Kernel_sqrt::Iso_rectangle_2 rect(bbox.xmin() - 1, bbox.ymin() - 1, bbox.xmax() + 1,
                                      bbox.ymax() + 1);

    auto vor = CroppedVoronoiFromDelaunay(rect);
    std::cout << "CroppedVoronoiFromDelaunay " << std::endl;

    vor.drawDual(sdg);
    std::cout << " sdg.drawDual " << std::endl;

    // we use a point set, in order to get back to original Kernel in an exact way
    CGAL::Point_set_2<Kernel> ptSet;

    ptSet.insert(pointsK.begin(), pointsK.end());

    From_sqrt_kernel fromSqrtKernel;
    _result.reserve(vor.m_cropped_vd.size());

    for (const Kernel_sqrt::Segment_2& segment : vor.m_cropped_vd) {
        const Kernel::Segment_2 kernelSeg = fromSqrtKernel(segment);
        _result.emplace_back(kernelSeg);
    }

    std::cout << "end sdg.drawDual " << std::endl;
}

VoronoiMedialSkeleton::VoronoiMedialSkeleton(const CGAL::Polygon_with_holes_2<Kernel>& polygon) {

    std::cout << "add polygon with hole : " << polygon.number_of_holes() << std::endl;

    SDG2 sdg;
    addPolygon(polygon, sdg);

    std::cout << "start cropping" << std::endl;
    auto bbox = polygon.outer_boundary().bbox();

    Kernel_sqrt::Iso_rectangle_2 rect(bbox.xmin() - 1, bbox.ymin() - 1, bbox.xmax() + 1,
                                      bbox.ymax() + 1);

    auto vor = CroppedVoronoiFromDelaunay(rect);
    std::cout << "CroppedVoronoiFromDelaunay " << std::endl;

    vor.drawDual(sdg);
    std::cout << " sdg.drawDual " << std::endl;
    From_sqrt_kernel fromSqrtKernel;
    for (const auto& segment : vor.m_cropped_vd) {
        _result.emplace_back(fromSqrtKernel(segment));
    }
}

auto VoronoiMedialSkeleton::getCachePoint(const Point_2& point,
                                          CGAL::Point_set_2<Kernel>& pointSet) -> const Point_2 {
    auto vertexHandle = pointSet.nearest_neighbor(point);

    // TODO Epsilon parameter : to put in the config

    double epsilon = 0.0000001;
    epsilon *= epsilon;
    CGAL::Comparison_result compare =
        CGAL::compare_squared_distance(point, vertexHandle->point(), epsilon);

    if (compare == CGAL::LARGER) {
        pointSet.insert(point);
        return point;
    }
    return vertexHandle->point();
}

auto VoronoiMedialSkeleton::snapPoint(const Point_2& point,
                                      CGAL::Point_set_2<Kernel>& pointSet) -> bool {
    auto vertexHandle = pointSet.nearest_neighbor(point);

    // TODO Epsilon parameter : to put in the config

    double epsilon = 0.0000001;
    epsilon *= epsilon;
    CGAL::Comparison_result compare =
        CGAL::compare_squared_distance(point, vertexHandle->point(), epsilon);

    return (compare == CGAL::SMALLER);
}

auto VoronoiMedialSkeleton::getConstCachePoint(
    const Point_2& point, CGAL::Point_set_2<Kernel>& pointSet) -> const Point_2& {
    return pointSet.nearest_neighbor(point)->point();
}

void VoronoiMedialSkeleton::setCachePoint(const Point_2& point, CGAL::Point_set_2<Kernel>& pointSet,
                                          std::map<Point_2, Point_2>& pointCache) {
    auto vertexHandle = pointSet.nearest_neighbor(point);

    // TODO Epsilon parameter : to put in the config

    if (vertexHandle == nullptr) {
        pointSet.insert(point);
        pointCache.emplace(point, point);
        return;
    }

    double epsilon = 0.000001;
    epsilon *= epsilon;

    CGAL::Comparison_result compare =
        CGAL::compare_squared_distance(point, vertexHandle->point(), epsilon);

    if (compare == CGAL::LARGER) {
        pointSet.insert(point);
        pointCache.emplace(point, point);
    } else {
        pointCache.emplace(point, vertexHandle->point());
    }
}

std::vector<basic::SegmentNode>
VoronoiMedialSkeleton::snapRounding(const std::vector<basic::SegmentNode>& result2,
                                    const basic::Arrangement_2Node& polygonArrangement) {

    std::vector<basic::SegmentNode> result;
    CGAL::Point_set_2<Kernel> ptSet;
    std::vector<Point_2> pointList;
    for (const auto& vertex : RangeHelper::make(polygonArrangement.vertices_begin(),
                                                polygonArrangement.vertices_end())) {
        pointList.emplace_back(vertex.point());
    }
    ptSet.insert(pointList.begin(), pointList.end());
    for (const basic::SegmentNode& curve : result2) {
        const Point_2 sourcePoint = getCachePoint(curve.source(), ptSet);
        const Point_2 targetPoint = getCachePoint(curve.target(), ptSet);
        CGAL::Comparison_result compare =
            CGAL::compare_squared_distance(sourcePoint, targetPoint, 0.);

        if (compare == CGAL::LARGER) {
            result.emplace_back(sourcePoint, targetPoint);
        }
    }
    return result;
}

auto VoronoiMedialSkeleton::snapRoundingArr(CGAL::Point_set_2<Kernel> pointSet,
                                            const Arrangement_2& arr) const -> Arrangement_2 {

    std::vector<Segment_info_2> result2;
    std::cout << " populating cache " << std::endl;

    std::map<Point_2, Point_2> mapCache;

    for (const Vertex& vertex : RangeHelper::make(arr.vertices_begin(), arr.vertices_end())) {
        setCachePoint(vertex.point(), pointSet, mapCache);
    }
    std::cout << " cache populated " << std::endl;
    for (const Halfedge& halfedge : RangeHelper::make(arr.edges_begin(), arr.edges_end())) {
        const Point_2& sourcePoint = mapCache.at(halfedge.source()->point());
        const Point_2& targetPoint = mapCache.at(halfedge.target()->point());
        CGAL::Comparison_result compare =
            CGAL::compare_squared_distance(sourcePoint, targetPoint, 0.);
        if (compare == CGAL::LARGER) {
            result2.emplace_back(Segment_2(sourcePoint, targetPoint), halfedge.curve().data());
            Kernel::Segment_2 segment = Segment_2(sourcePoint, targetPoint);
            if (segment.squared_length() < 0.0001) {
                std::cout << "seg  Segment_2(sourcePoint, targetPoint) is short " << segment
                          << " l " << segment.squared_length() << "\n";
            }
        }
    }
    std::cout << " segment list result2 ok " << std::endl;
    Arrangement_2 arr2;
    CGAL::insert(arr2, result2.begin(), result2.end());
    return arr2;
}

auto VoronoiMedialSkeleton::extendsToProjection(const Kernel::Point_2& projectionPoint,
                                                const Halfedge& halfedge) -> Point_2 {

    Kernel::FT minDistance = 100;
    Kernel::Segment_2 segment;

    for (const Halfedge& currentHalfedge : RangeHelper::make(halfedge.ccb())) {
        Kernel::FT distance =
            CGAL::squared_distance(projectionPoint, Kernel::Segment_2(currentHalfedge.curve()));
        if (distance < minDistance and distance > 0) {
            minDistance = distance;
            segment = currentHalfedge.curve();
        }
    }
    if (minDistance < 100) {
        const Kernel::Line_2 line(segment);
        return line.projection(projectionPoint);
    }

    return projectionPoint;
}

auto VoronoiMedialSkeleton::removeAntennaArr(const Arrangement_2& arr) -> Arrangement_2 {
    std::vector<Segment_info_2> resultConvert;
    for (Edge_const_iterator eit = arr.edges_begin(); eit != arr.edges_end(); ++eit) {
        const Halfedge& he2 = *eit;
        if ((&*he2.face() != &*he2.twin()->face()) or
            Kernel::Segment_2(he2.curve()).squared_length() > 1e-50) {
            resultConvert.emplace_back(he2.curve());
        }
    }
    Arrangement_2 arrWithoutAntenna;
    CGAL::insert(arrWithoutAntenna, resultConvert.begin(), resultConvert.end());
    return arrWithoutAntenna;
}

auto VoronoiMedialSkeleton::extendsAntennaArr(const Arrangement_2& arr) -> Arrangement_2 {
    std::map<Point_2, Point_2> pmap;

    std::vector<Segment_info_2> resultConvert;
    for (Edge_const_iterator eit = arr.edges_begin(); eit != arr.edges_end(); ++eit) {
        const Halfedge& he2 = *eit;
        if ((&*he2.face() == &*he2.twin()->face())) {
            Kernel::Point_2 source = he2.source()->point();
            Kernel::Point_2 target = he2.target()->point();
            if (he2.source()->degree() < 2) {
                source = extendsToProjection(source, he2);
                pmap.emplace(source, he2.source()->point());
            }
            if (he2.target()->degree() < 2) {
                target = extendsToProjection(target, he2);
                pmap.emplace(target, he2.target()->point());
            }

            CGAL::Comparison_result compare = CGAL::compare_squared_distance(source, target, 0.);
            if (compare == CGAL::LARGER) {
                resultConvert.emplace_back(Segment_info_2(
                    Kernel::Segment_2(source, target),
                    EdgeInfo(he2.curve().data().direction(), EdgeInfo::Coordinate{0})));
            }
        } else {
            resultConvert.emplace_back(he2.curve());
        }
    }
    Arrangement_2 arrWithoutAntenna;
    CGAL::insert(arrWithoutAntenna, resultConvert.begin(), resultConvert.end());

    resultConvert.clear();

    for (const Halfedge& halfedge :
         RangeHelper::make(arrWithoutAntenna.edges_begin(), arrWithoutAntenna.edges_end())) {

        Point_2 sourcePoint;
        {
            auto ite = pmap.find(halfedge.source()->point());
            if (ite != pmap.end()) {
                sourcePoint = ite->second;
            } else {
                sourcePoint = halfedge.source()->point();
            }
        }

        Point_2 targetPoint;
        {
            auto ite = pmap.find(halfedge.target()->point());
            if (ite != pmap.end()) {
                targetPoint = ite->second;
            } else {
                targetPoint = halfedge.target()->point();
            }
        }

        CGAL::Comparison_result compare =
            CGAL::compare_squared_distance(sourcePoint, targetPoint, 0.);
        if (compare == CGAL::LARGER) {
            resultConvert.emplace_back(Segment_2(sourcePoint, targetPoint),
                                       halfedge.curve().data());
        }
    }
    Arrangement_2 arr2;
    CGAL::insert(arr2, resultConvert.begin(), resultConvert.end());
    return arr2;
}

auto VoronoiMedialSkeleton::getSimpleArr(const Ribbon& rib,
                                         const Ribbon& ribLimit) const -> Arrangement_2 {
    std::vector<Segment_info_2> resultConvert;

    rib.addToSegments(resultConvert);
    std::vector<Point_2> pointsK = rib.getPoints();

    for (const Kernel::Segment_2& segment : _result) {
        resultConvert.emplace_back(segment, EdgeInfo(rib.fillColor() + 1, EdgeInfo::Coordinate{0}));
    }
    for (const Kernel::Segment_2& segment : ribLimit.getSegments()) {
        resultConvert.emplace_back(segment, EdgeInfo(rib.fillColor() + 1, EdgeInfo::Coordinate{0}));
    }

    Arrangement_2 arr;
    CGAL::insert(arr, resultConvert.begin(), resultConvert.end());

    CGAL::Point_set_2<Kernel> ptSet;
    ptSet.insert(pointsK.begin(), pointsK.end());

    Arrangement_2 arr2 = snapRoundingArr(ptSet, arr);

    arr2 = extendsAntennaArr(arr2);

    arr2 = snapRoundingArr(ptSet, arr2);
    std::cout << " arr2 ok " << std::endl;
    return arr2;
}

auto VoronoiMedialSkeleton::cutAndGetArrangementSkeleton(
    const CGAL::Polygon_with_holes_2<Kernel>& polygon) const -> basic::Arrangement_2Node {

    basic::Polygon_with_holes_2Node pHoleNode;

    for (const auto& hole : RangeHelper::make(polygon.holes_begin(), polygon.holes_end())) {
        basic::Polygon_2Node polygon_node;

        polygon_node.insert(polygon_node.vertices_end(), hole.vertices_begin(),
                            hole.vertices_end());
        pHoleNode.add_hole(polygon_node);
    }
    const CGAL::Polygon_2<Kernel>& outer = polygon.outer_boundary();

    pHoleNode.outer_boundary().insert(pHoleNode.outer_boundary().vertices_end(),
                                      outer.vertices_begin(), outer.vertices_end());

    basic::Polygon_set_2Node polygonSet(pHoleNode);

    basic::Arrangement_2Node& arr = polygonSet.arrangement();

    for (basic::FaceNode& face : RangeHelper::make(arr.faces_begin(), arr.faces_end())) {
        // the test if this is a hole or not is done inside setPolygonId
        if (face.contained()) {
            face.setPolygonId(+1);
            for (basic::HalfedgeNode& halfedge : RangeHelper::make(face.outer_ccb())) {
                (void)halfedge; // edge data no longer stored on curves in CGAL 5.x
            }
            for (auto iter = face.inner_ccbs_begin(); iter != face.inner_ccbs_end(); ++iter) {
                for (basic::HalfedgeNode& halfedge : RangeHelper::make(*iter)) {
                    (void)halfedge; // edge data no longer stored on curves in CGAL 5.x
                }
            }
        }
    }

    std::vector<basic::SegmentNode> resultConvert;

    for (const Kernel::Segment_2& segment : _result) {
        resultConvert.emplace_back(segment);
    }

    basic::Arrangement_2Node arr2;
    CGAL::insert(arr2, resultConvert.begin(), resultConvert.end());

    basic::Overlay_traitsNode overlayTraits;
    basic::Arrangement_2Node arr3;

    CGAL::overlay(arr, arr2, arr3, overlayTraits);

    // remove inner  antenna
    std::vector<basic::SegmentNode> result2;

    for (basic::HalfedgeNode& halfedge : RangeHelper::make(arr3.edges_begin(), arr3.edges_end())) {

        if (halfedge.face()->data().count(1) == 1 or
            halfedge.twin()->face()->data().count(1) == 1) {
            result2.emplace_back(halfedge.curve());
        }
    }

    auto snap = snapRounding(result2, arr);

    basic::Arrangement_2Node arr4;
    CGAL::insert(arr4, snap.begin(), snap.end());

    basic::Overlay_traitsNode overlayTraits2;
    basic::Arrangement_2Node arr5;
    CGAL::overlay(arr, arr4, arr5, overlayTraits2);
    return arr5;
}

} /* namespace laby */
