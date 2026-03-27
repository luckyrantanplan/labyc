/*
 * VoronoiSeg.cpp
 *
 *  Created on: Oct 25, 2017
 *      Author: florian
 */

// standard includes
#include "VoronoiMedialSkeleton.h"

#include <boost/optional/optional.hpp>
#include <boost/variant/get.hpp>
#include <boost/variant/variant.hpp>
#include <CGAL/Arr_dcel_base.h>
#include <CGAL/Arr_geometry_traits/Consolidated_curve_data_aux.h>
#include <CGAL/Arr_geometry_traits/Curve_data_aux.h>
#include <CGAL/Arr_overlay_2.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arrangement_2/Arrangement_on_surface_2_global.h>
#include <CGAL/Arrangement_on_surface_2.h>
#include <CGAL/Bbox_2.h>
#include <CGAL/Boolean_set_operations_2/Gps_default_dcel.h>
#include <CGAL/Cartesian_converter.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/enum.h>
#include <CGAL/General_polygon_set_2.h>
#include <CGAL/General_polygon_with_holes_2.h>
#include <CGAL/Iterator_transform.h>
#include <CGAL/Kernel/global_functions_2.h>
#include <CGAL/intersection_2.h>
#include <CGAL/number_utils.h>
#include <CGAL/Point_2.h>
#include <CGAL/Segment_2.h>
#include <CGAL/Triangulation_vertex_base_2.h>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "agg/agg_curves.h"
#include "basic/KernelConverter.h"
#include "basic/RangeHelper.h"
#include "Ribbon.h"

namespace laby {
class Ribbon;
} /* namespace laby */

namespace laby {

Kernel_sqrt::Line_2 Cropped_voronoi_from_delaunay::CustomParabola::get_tangent(const Point_2& p) {
    if (p == o) {
        return Line_2(o, l.direction()); // line qui passe par o et qui a pour direction l
    }
    Line_2 p1h(l.projection(p), p);
    Line_2 cP1(c, p);
    return CGAL::bisector(p1h, cP1);
}

Cropped_voronoi_from_delaunay::CustomParabola::Point_2 Cropped_voronoi_from_delaunay::CustomParabola::computeBezier() {

    if (p1 == p2) {
        return p1;
    }

    Kernel_sqrt::Line_2 tangent1 = get_tangent(p1);
    Kernel_sqrt::Line_2 tangent2 = get_tangent(p2);
    auto result4 = CGAL::intersection(tangent1, tangent2);

    if (!result4) {
        std::cout << " no intersection " << std::endl;
        return CGAL::midpoint(p1, p2);
    }
    if (const Kernel_sqrt::Point_2* bezier_control_point = boost::get<Kernel_sqrt::Point_2>(&*result4)) {
        return *bezier_control_point;
    }
    else {
        std::cout << "no bezier_control_point " << *result4 << std::endl;
    };
    return CGAL::midpoint(p1, p2);
}

void Cropped_voronoi_from_delaunay::draw_dual(const SDG2& sdg) {
    To_sqrt_kernel to_sqrt_kernel;

    for (const auto& edge : RangeHelper::make(sdg.finite_edges_begin(), sdg.finite_edges_end())) {

        Line_2 l;
        Segment_2 s;
        Ray_2 r;
        CGAL::Parabola_segment_2<Gt> ps;
        CGAL::Object o = sdg.primal(edge);
        if (CGAL::assign(l, o)) {
            //        crop_and_extract_segment(l);
        }

        if (CGAL::assign(s, o)) {
            crop_and_extract_segment(s);
        }
        if (CGAL::assign(r, o)) {
            //        crop_and_extract_segment(r);
        }
        if (CGAL::assign(ps, o)) {
            CustomParabola cpara(ps);
            auto cp = cpara.computeBezier();

            auto a = cpara.getP1();
            auto b = cpara.getP2();
            agg::Curve3 curve(CGAL::to_double(a.x()), CGAL::to_double(a.y()),   //
                              CGAL::to_double(cp.x()), CGAL::to_double(cp.y()), //
                              CGAL::to_double(b.x()), CGAL::to_double(b.y()));

            Kernel_sqrt::Point_2 previous = a;
            auto& pts = curve.getPoints();
            for (std::size_t i = 1; i < pts.size() - 1; ++i) {
                Kernel_sqrt::Point_2 p = to_sqrt_kernel(pts.at(i));
                crop_and_extract_segment(Segment_2(previous, p));
                previous = p;
            }
            crop_and_extract_segment(Segment_2(previous, b));
        }
    }
}

void VoronoiMedialSkeleton::addSimplePolygon(const CGAL::Polygon_2<Kernel>& outerBoundary, std::vector<Gt::Point_2>& points,
                                             std::vector<std::pair<std::size_t, std::size_t>>& indices) {
    SDG2::Site_2 site;
    std::size_t k = points.size();

    std::size_t save_k = points.size();
    To_sqrt_kernel to_sqrt_kernel;
    CGAL::Polygon_2<Kernel>::Edge_const_iterator ite = outerBoundary.edges_begin();
    site = SDG2::Site_2::construct_site_2(to_sqrt_kernel(ite->source()), to_sqrt_kernel(ite->target()));

    points.push_back(site.source_of_supporting_site());
    for (const CGAL::Segment_2<Kernel> seg : RangeHelper::make(outerBoundary.edges_begin(), outerBoundary.edges_end())) {
        site = SDG2::Site_2::construct_site_2(to_sqrt_kernel(seg.source()), to_sqrt_kernel(seg.target()));
        indices.push_back(std::make_pair(k, k + 1));
        points.push_back(site.target_of_supporting_site());
        ++k;
    }
    indices.push_back(std::make_pair(k, save_k));
}

void VoronoiMedialSkeleton::addPolyline(const Polyline& pl, std::vector<Gt::Point_2>& points, std::vector<std::pair<std::size_t, std::size_t>>& indices) {
    const std::vector<Point_2>& pts = pl.points;

    if (pts.size() < 2) {
        return;
    }
    SDG2::Site_2 site;
    std::size_t k = points.size();

    std::size_t save_k = points.size();
    To_sqrt_kernel to_sqrt_kernel;

    site = SDG2::Site_2::construct_site_2(to_sqrt_kernel(pts.at(0)), to_sqrt_kernel(pts.at(1)));
    points.push_back(site.source_of_supporting_site());

    for (std::size_t i = 1; i < pts.size(); ++i) {
        const Point_2& source = pts.at(i - 1);
        const Point_2& target = pts.at(i);
        site = SDG2::Site_2::construct_site_2(to_sqrt_kernel(source), to_sqrt_kernel(target));
        indices.push_back(std::make_pair(k, k + 1));
        points.push_back(site.target_of_supporting_site());
        ++k;
    }
    if (pl.closed) {
        indices.push_back(std::make_pair(k, save_k));
    }
}

void VoronoiMedialSkeleton::addPolygon(const CGAL::Polygon_with_holes_2<Kernel>& polygon, SDG2& sdg) {

    std::vector<Gt::Point_2> points;
    // segments of the polygon as a pair of point indices
    std::vector<std::pair<std::size_t, std::size_t>> indices;

    addSimplePolygon(polygon.outer_boundary(), points, indices);

    for (const auto& hole : RangeHelper::make(polygon.holes_begin(), polygon.holes_end())) {
        addSimplePolygon(hole, points, indices);
    }
    sdg.insert_segments(points.begin(), points.end(), indices.begin(), indices.end());
}

VoronoiMedialSkeleton::VoronoiMedialSkeleton(const std::vector<Segment_info_2>& seg_list, const CGAL::Bbox_2& bbox) {
    SDG2 sdg;
    std::vector<SDG2::Site_2> sites;
    // segments of the polygon as a pair of point indices
    std::vector<Kernel::Point_2> pointsK;
    To_sqrt_kernel to_sqrt_kernel;

    for (const Segment_info_2& seg : seg_list) {
        sites.emplace_back(SDG2::Site_2::construct_site_2(to_sqrt_kernel(seg.source()), to_sqrt_kernel(seg.target())));
        pointsK.emplace_back(seg.source());
        pointsK.emplace_back(seg.target());
    }

    sdg.insert_segments(sites.begin(), sites.end());
    std::cout << "start cropping" << std::endl;

    Kernel_sqrt::Iso_rectangle_2 rect(bbox.xmin() - 1, bbox.ymin() - 1, bbox.xmax() + 1, bbox.ymax() + 1);

    Cropped_voronoi_from_delaunay vor(rect);
    std::cout << "Cropped_voronoi_from_delaunay " << std::endl;

    vor.draw_dual(sdg);
    std::cout << " sdg.draw_dual " << std::endl;

    // we use a point set, in order to get back to original Kernel in an exact way
    CGAL::Point_set_2<Kernel> pt_set;

    pt_set.insert(pointsK.begin(), pointsK.end());

    From_sqrt_kernel from_sqrt_kernel;
    for (const auto& s : vor.m_cropped_vd) {
        auto kernelSeg = from_sqrt_kernel(s);
        const Point_2 a = getCachePoint(kernelSeg.source(), pt_set);
        const Point_2 b = getCachePoint(kernelSeg.target(), pt_set);
        CGAL::Comparison_result compare = CGAL::compare_squared_distance(a, b, 0.);

        if (compare == CGAL::LARGER) {
            _result.emplace_back(Segment_2(a, b));
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
    for (const Polyline& pl : ribbon.lines()) {
        addPolyline(pl, points, indices);
        pointsK.insert(pointsK.end(), pl.points.begin(), pl.points.end());
    }
    sdg.insert_segments(points.begin(), points.end(), indices.begin(), indices.end());
    std::cout << "start cropping" << std::endl;

    Kernel_sqrt::Iso_rectangle_2 rect(bbox.xmin() - 1, bbox.ymin() - 1, bbox.xmax() + 1, bbox.ymax() + 1);

    Cropped_voronoi_from_delaunay vor(rect);
    std::cout << "Cropped_voronoi_from_delaunay " << std::endl;

    vor.draw_dual(sdg);
    std::cout << " sdg.draw_dual " << std::endl;

    // we use a point set, in order to get back to original Kernel in an exact way
    CGAL::Point_set_2<Kernel> pt_set;

    pt_set.insert(pointsK.begin(), pointsK.end());

    From_sqrt_kernel from_sqrt_kernel;
    _result.reserve(vor.m_cropped_vd.size());

    for (const Kernel_sqrt::Segment_2& s : vor.m_cropped_vd) {

        const Kernel::Segment_2 kernelSeg = from_sqrt_kernel(s);
        _result.emplace_back(kernelSeg);
    }

    std::cout << "end sdg.draw_dual " << std::endl;
}

VoronoiMedialSkeleton::VoronoiMedialSkeleton(const CGAL::Polygon_with_holes_2<Kernel>& polygon) {

    std::cout << "add polygon with hole : " << polygon.number_of_holes() << std::endl;

    SDG2 sdg;
    addPolygon(polygon, sdg);

    std::cout << "start cropping" << std::endl;
    auto bbox = polygon.outer_boundary().bbox();

    Kernel_sqrt::Iso_rectangle_2 rect(bbox.xmin() - 1, bbox.ymin() - 1, bbox.xmax() + 1, bbox.ymax() + 1);

    Cropped_voronoi_from_delaunay vor(rect);
    std::cout << "Cropped_voronoi_from_delaunay " << std::endl;

    vor.draw_dual(sdg);
    std::cout << " sdg.draw_dual " << std::endl;
    From_sqrt_kernel from_sqrt_kernel;
    for (const auto& s : vor.m_cropped_vd) {
        _result.emplace_back(from_sqrt_kernel(s));
    }
}

const Point_2 VoronoiMedialSkeleton::getCachePoint(const Point_2& pt, CGAL::Point_set_2<Kernel>& pt_set) {
    auto vertex_handle = pt_set.nearest_neighbor(pt);

    // TODO Epsilon parameter : to put in the config

    double epsilon = 0.0000001;
    epsilon *= epsilon;
    CGAL::Comparison_result compare = CGAL::compare_squared_distance(pt, vertex_handle->point(), epsilon);

    if (compare == CGAL::LARGER) {
        pt_set.insert(pt);
        return pt;
    }
    return vertex_handle->point();
}

bool VoronoiMedialSkeleton::snapPoint(const Point_2& pt, CGAL::Point_set_2<Kernel>& pt_set) {
    auto vertex_handle = pt_set.nearest_neighbor(pt);

    // TODO Epsilon parameter : to put in the config

    double epsilon = 0.0000001;
    epsilon *= epsilon;
    CGAL::Comparison_result compare = CGAL::compare_squared_distance(pt, vertex_handle->point(), epsilon);

    return (compare == CGAL::SMALLER);
}

const Point_2& VoronoiMedialSkeleton::getConstCachePoint(const Point_2& pt, CGAL::Point_set_2<Kernel>& pt_set) {
    return pt_set.nearest_neighbor(pt)->point();
}

void VoronoiMedialSkeleton::setCachePoint(const Point_2& pt, CGAL::Point_set_2<Kernel>& pt_set, std::map<Point_2, Point_2>& map_cache) {
    auto vertex_handle = pt_set.nearest_neighbor(pt);

    // TODO Epsilon parameter : to put in the config

    if (vertex_handle == nullptr) {
        pt_set.insert(pt);
        map_cache.emplace(pt, pt);
        return;
    }

    double epsilon = 0.000001;
    epsilon *= epsilon;

    CGAL::Comparison_result compare = CGAL::compare_squared_distance(pt, vertex_handle->point(), epsilon);

    if (compare == CGAL::LARGER) {
        pt_set.insert(pt);
        map_cache.emplace(pt, pt);
    }
    else {
        map_cache.emplace(pt, vertex_handle->point());
    }
}

std::vector<basic::SegmentNode> VoronoiMedialSkeleton::snap_rounding(const std::vector<basic::SegmentNode>& result2,
                                                                     const basic::Arrangement_2Node& polygon_arr) {

    std::vector<basic::SegmentNode> result;
    CGAL::Point_set_2<Kernel> pt_set;
    std::vector<Point_2> point_list;
    for (const auto& v : RangeHelper::make(polygon_arr.vertices_begin(), polygon_arr.vertices_end())) {
        point_list.emplace_back(v.point());
    }
    pt_set.insert(point_list.begin(), point_list.end());
    for (const basic::SegmentNode& curve : result2) {
        const Point_2 a = getCachePoint(curve.source(), pt_set);
        const Point_2 b = getCachePoint(curve.target(), pt_set);
        CGAL::Comparison_result compare = CGAL::compare_squared_distance(a, b, 0.);

        if (compare == CGAL::LARGER) {
            result.emplace_back(a, b);
        }
    }
    return result;
}

Arrangement_2 VoronoiMedialSkeleton::snapRoundingArr(CGAL::Point_set_2<Kernel> pt_set, const Arrangement_2& arr) const {

    std::vector<Segment_info_2> result2;
    std::cout << " populating cache " << std::endl;

    std::map<Point_2, Point_2> mapCache;

    for (const Vertex& v : RangeHelper::make(arr.vertices_begin(), arr.vertices_end())) {
        setCachePoint(v.point(), pt_set, mapCache);
    }
    std::cout << " cache populated " << std::endl;
    for (const Halfedge& he : RangeHelper::make(arr.edges_begin(), arr.edges_end())) {
        const Point_2& a = mapCache.at(he.source()->point());
        const Point_2& b = mapCache.at(he.target()->point());
        CGAL::Comparison_result compare = CGAL::compare_squared_distance(a, b, 0.);
        if (compare == CGAL::LARGER) {
            result2.emplace_back(Segment_2(a, b), he.curve().data());
            Kernel::Segment_2 seg = Segment_2(a, b);
            if (seg.squared_length() < 0.0001) {
                std::cout << "seg  Segment_2(a, b) is short " << seg << " l " << seg.squared_length() << "\n";
            }
        }
    }
    std::cout << " segment list result2 ok " << std::endl;
    Arrangement_2 arr2;
    CGAL::insert(arr2, result2.begin(), result2.end());
    return arr2;
}

Point_2 VoronoiMedialSkeleton::extends_to_projection(const Kernel::Point_2& a, const Halfedge& he2) {

    Kernel::FT min = 100;
    Kernel::Segment_2 s;

    for (const Halfedge& he : RangeHelper::make(he2.ccb())) {
        Kernel::FT dist = CGAL::squared_distance(a, Kernel::Segment_2(he.curve()));
        if (dist < min and dist > 0) {
            min = dist;
            s = he.curve();
        }
    }
    if (min < 100) {
        const Kernel::Line_2 line(s);
        return line.projection(a);
    }

    return a;
}

Arrangement_2 VoronoiMedialSkeleton::removeAntennaArr(const Arrangement_2& arr) {
    std::vector<Segment_info_2> result_convert;
    for (Edge_const_iterator eit = arr.edges_begin(); eit != arr.edges_end(); ++eit) {
        const Halfedge& he2 = *eit;
        if ((&*he2.face() != &*he2.twin()->face()) or Kernel::Segment_2(he2.curve()).squared_length() > 1e-50) {
            result_convert.emplace_back(he2.curve());
        }
    }
    Arrangement_2 arr_without_antenna;
    CGAL::insert(arr_without_antenna, result_convert.begin(), result_convert.end());
    return arr_without_antenna;
}

Arrangement_2 VoronoiMedialSkeleton::extendsAntennaArr(const Arrangement_2& arr) {
    std::map<Point_2, Point_2> pmap;

    std::vector<Segment_info_2> result_convert;
    for (Edge_const_iterator eit = arr.edges_begin(); eit != arr.edges_end(); ++eit) {
        const Halfedge& he2 = *eit;
        if ((&*he2.face() == &*he2.twin()->face())) {
            Kernel::Point_2 source = he2.source()->point();
            Kernel::Point_2 target = he2.target()->point();
            if (he2.source()->degree() < 2) {
                source = extends_to_projection(source, he2);
                pmap.emplace(source, he2.source()->point());
            }
            if (he2.target()->degree() < 2) {
                target = extends_to_projection(target, he2);
                pmap.emplace(target, he2.target()->point());
            }

            CGAL::Comparison_result compare = CGAL::compare_squared_distance(source, target, 0.);
            if (compare == CGAL::LARGER) {
                result_convert.emplace_back(
                    Segment_info_2(Kernel::Segment_2(source, target), EdgeInfo(he2.curve().data().direction(), EdgeInfo::Coordinate{0})));
            }
        }
        else {
            result_convert.emplace_back(he2.curve());
        }
    }
    Arrangement_2 arr_without_antenna;
    CGAL::insert(arr_without_antenna, result_convert.begin(), result_convert.end());

    result_convert.clear();

    for (const Halfedge& he : RangeHelper::make(arr_without_antenna.edges_begin(), arr_without_antenna.edges_end())) {

        Point_2 a;
        {
            auto ite = pmap.find(he.source()->point());
            if (ite != pmap.end()) {
                a = ite->second;
            }
            else {
                a = he.source()->point();
            }
        }

        Point_2 b;
        {
            auto ite = pmap.find(he.target()->point());
            if (ite != pmap.end()) {
                b = ite->second;
            }
            else {
                b = he.target()->point();
            }
        }

        CGAL::Comparison_result compare = CGAL::compare_squared_distance(a, b, 0.);
        if (compare == CGAL::LARGER) {
            result_convert.emplace_back(Segment_2(a, b), he.curve().data());
        }
    }
    Arrangement_2 arr2;
    CGAL::insert(arr2, result_convert.begin(), result_convert.end());
    return arr2;
}

Arrangement_2 VoronoiMedialSkeleton::getSimpleArr(const Ribbon& rib, const Ribbon& ribLimit) const {
    std::vector<Segment_info_2> result_convert;

    rib.addToSegments(result_convert);
    std::vector<Point_2> pointsK = rib.getPoints();

    for (const Kernel::Segment_2& s : _result) {
        result_convert.emplace_back(s, EdgeInfo(rib.fillColor() + 1, EdgeInfo::Coordinate{0}));
    }
    for (const Kernel::Segment_2& s : ribLimit.getSegments()) {
        result_convert.emplace_back(s, EdgeInfo(rib.fillColor() + 1, EdgeInfo::Coordinate{0}));
    }

    Arrangement_2 arr;
    CGAL::insert(arr, result_convert.begin(), result_convert.end());

    CGAL::Point_set_2<Kernel> pt_set;
    pt_set.insert(pointsK.begin(), pointsK.end());

    Arrangement_2 arr2 = snapRoundingArr(pt_set, arr);

    arr2 = extendsAntennaArr(arr2);

    arr2 = snapRoundingArr(pt_set, arr2);
    std::cout << " arr2 ok " << std::endl;
    return arr2;
}

basic::Arrangement_2Node VoronoiMedialSkeleton::cutAndGetArrangementSkeleton(const CGAL::Polygon_with_holes_2<Kernel>& polygon) const {

    basic::Polygon_with_holes_2Node p_hole_node;

    for (const auto& hole : RangeHelper::make(polygon.holes_begin(), polygon.holes_end())) {
        basic::Polygon_2Node polygon_node;

        polygon_node.insert(polygon_node.vertices_end(), hole.vertices_begin(), hole.vertices_end());
        p_hole_node.add_hole(polygon_node);
    }
    const CGAL::Polygon_2<Kernel>& outer = polygon.outer_boundary();

    p_hole_node.outer_boundary().insert(p_hole_node.outer_boundary().vertices_end(), outer.vertices_begin(), outer.vertices_end());

    basic::Polygon_set_2Node polygon_set(p_hole_node);

    basic::Arrangement_2Node& arr = polygon_set.arrangement();

    for (basic::FaceNode& face : RangeHelper::make(arr.faces_begin(), arr.faces_end())) {
        // the test if this is a hole or not is done inside setPolygonId
        if (face.contained()) {
            face.setPolygonId(+1);
            for (basic::HalfedgeNode& he : RangeHelper::make(face.outer_ccb())) {
                (void)he; // edge data no longer stored on curves in CGAL 5.x
            }
            for (auto iter = face.inner_ccbs_begin(); iter != face.inner_ccbs_end(); ++iter) {
                for (basic::HalfedgeNode& he : RangeHelper::make(*iter)) {
                    (void)he; // edge data no longer stored on curves in CGAL 5.x
                }
            }
        }
    }

    std::vector<basic::SegmentNode> result_convert;

    for (const Kernel::Segment_2& s : _result) {

        result_convert.emplace_back(s);
    }

    basic::Arrangement_2Node arr2;
    CGAL::insert(arr2, result_convert.begin(), result_convert.end());

    basic::Overlay_traitsNode overlay_traits;
    basic::Arrangement_2Node arr3;

    CGAL::overlay(arr, arr2, arr3, overlay_traits);

    // remove inner  antenna
    std::vector<basic::SegmentNode> result2;

    for (basic::HalfedgeNode& he : RangeHelper::make(arr3.edges_begin(), arr3.edges_end())) {

        if (he.face()->data().count(1) == 1 or he.twin()->face()->data().count(1) == 1) {
            result2.emplace_back(he.curve());
        }
    }

    auto snap = snap_rounding(result2, arr);

    basic::Arrangement_2Node arr4;
    CGAL::insert(arr4, snap.begin(), snap.end());

    basic::Overlay_traitsNode overlay_traits2;
    basic::Arrangement_2Node arr5;
    CGAL::overlay(arr, arr4, arr5, overlay_traits2);
    return arr5;
}

} /* namespace laby */
