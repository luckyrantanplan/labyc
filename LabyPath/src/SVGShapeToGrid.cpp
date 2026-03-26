/*
 * SVGShapeToGrid.cpp
 *
 *  Created on: Jun 21, 2018
 *      Author: florian
 */

#include "SVGShapeToGrid.h"

#include <CGAL/Arr_dcel_base.h>
#include <CGAL/Arr_geometry_traits/Consolidated_curve_data_aux.h>
#include <CGAL/Arr_geometry_traits/Curve_data_aux.h>
#include <CGAL/Arrangement_on_surface_2.h>
#include <CGAL/create_offset_polygons_2.h>
#include <CGAL/create_straight_skeleton_from_polygon_with_holes_2.h>
#include <CGAL/Direction_2.h>
#include <CGAL/Iterator_transform.h>
#include <CGAL/Kernel/global_functions_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_set_2.h>
#include <iostream>
#include <iterator>

#include "basic/RangeHelper.h"

namespace laby {

void SVGShapeToGrid::markFace(Halfedge& he, Face& parent_face, std::queue<Face*>& q) {
    FaceNonZeroData& data = he.face()->data();
    if (!data._walked) {
        int32_t winding_number = parent_face.data()._winding_number;
        const Halfedge::X_monotone_curve& curve = he.curve();
        const CGAL::_Unique_list<Segment_2 *> curve_data = curve.data();

        for (const Segment_2 * ocit : RangeHelper::make(curve_data.begin(), curve_data.end())) {
            const Segment_2& seg = *ocit;

            if (CGAL::Direction_2<Kernel>(seg) == CGAL::Direction_2<Kernel>(he.target()->point() - he.source()->point())) {
                ++winding_number;
            } else {
                --winding_number;
            }

        }
        data._walked = true;
        data._winding_number = winding_number;
        q.push(&*he.face());
    }

}

void SVGShapeToGrid::markWindingRule(Arr_with_hist_2& arr) {
    std::queue<Face*> q;
    arr.unbounded_face()->data()._walked = true;
    q.push(&*arr.unbounded_face());
    while (!q.empty()) {
        Face& face = *q.front();
        q.pop();
        for (auto iter = face.outer_ccbs_begin(); iter != face.outer_ccbs_end(); ++iter) {
            for (Halfedge& halfedge : RangeHelper::make(*iter)) {
                Halfedge& twin = *halfedge.twin();
                markFace(twin, face, q);
            }
        }
        for (auto iter = face.inner_ccbs_begin(); iter != face.inner_ccbs_end(); ++iter) {
            for (Halfedge& halfedge : RangeHelper::make(*iter)) {
                Halfedge& twin = *halfedge.twin();
                markFace(twin, face, q);
            }
        }
    }

}

void SVGShapeToGrid::debugPoly(const CGAL::Polygon_2<Kernel>& outP) {
    if (!outP.is_simple()) {
        std::cout << " outP.is_simple() " << outP.is_simple() << std::endl;
        for (const Kernel::Point_2& p : outP.container()) {
            std::cout << p << "\n";
        }
        std::cout << "\n";
    }
}

std::vector<CGAL::Polygon_with_holes_2<Kernel> > SVGShapeToGrid::get_polygons(Arr_with_hist_2& arr) {
    CGAL::Polygon_set_2<Kernel> poly_set;
    for (const Face& face : RangeHelper::make(arr.faces_begin(), arr.faces_end())) {
        if (face.data()._winding_number != 0) {
            CGAL::Polygon_2<Kernel> outP;
            for (const Halfedge& he : RangeHelper::make(face.outer_ccb())) {

                outP.push_back(he.source()->point());

            }
            std::vector<CGAL::Polygon_2<Kernel> > holesP;
            for (auto iter = face.inner_ccbs_begin(); iter != face.inner_ccbs_end(); ++iter) {
                holesP.emplace_back();
                for (const Halfedge& halfedge : RangeHelper::make(*iter)) {

                    holesP.back().push_back(halfedge.source()->point());

                }
                debugPoly(holesP.back());
            }
            CGAL::Polygon_with_holes_2<Kernel> poly_hole(outP, holesP.begin(), holesP.end());

            debugPoly(outP);

            poly_set.join(poly_hole);
        }
    }

    std::vector<CGAL::Polygon_2<Kernel> > patches;

    const auto& arrSet = poly_set.arrangement();
    CGAL::Vector_2<Kernel> vX(0.1, 0);
    CGAL::Vector_2<Kernel> vY(0, 0.1);
    for (const auto& vertex : RangeHelper::make(arrSet.vertices_begin(), arrSet.vertices_end())) {
        if (vertex.degree() > 2) {
            patches.emplace_back();
            CGAL::Polygon_2<Kernel>& lp = patches.back();
            const CGAL::Point_2<Kernel>& p = vertex.point();
            lp.push_back(p - vX - vY);
            lp.push_back(p + vX - vY);
            lp.push_back(p + vX + vY);
            lp.push_back(p - vX + vY);
        }
    }

    for (const CGAL::Polygon_2<Kernel>& lp : patches) {
        poly_set.join(lp);
    }

    std::vector<CGAL::Polygon_with_holes_2<Kernel> > res;
    std::cout << "The result contains " << poly_set.number_of_polygons_with_holes() << " components:" << std::endl;
    poly_set.polygons_with_holes(std::back_inserter(res));

    return res;
}

std::vector<Segment_2> SVGShapeToGrid::createOffset(std::vector<CGAL::Polygon_with_holes_2<Kernel> >& res, const double l) {
    std::cout << "SVGShapeToGrid::createOffset " << res.size() << std::endl;
    SsPtr iss = CGAL::create_interior_straight_skeleton_2(res.front());
    std::cout << "first step OK " << std::endl;
    std::vector<Segment_2> segResult;
    for (double ll = 0.01; ll < 10.; ll += l) {

        auto offset_polygons = CGAL::create_offset_polygons_2<CGAL::Polygon_2<InexactK> >(ll, *iss);
        for (const auto& poly_sp : offset_polygons) {
            const auto& poly = *poly_sp;
            for (auto eit = poly.edges_begin(); eit != poly.edges_end(); ++eit) {
                auto src = *eit;
                segResult.emplace_back(
                    Point_2(CGAL::to_double(src.source().x()), CGAL::to_double(src.source().y())),
                    Point_2(CGAL::to_double(src.target().x()), CGAL::to_double(src.target().y()))
                );
            }
        }
    }
    std::cout << "SVGShapeToGrid::createOffset end" << std::endl;
    return segResult;
}

std::vector<Segment_2> SVGShapeToGrid::addToSegments(const Ribbon& ribb) {
    std::vector<Segment_2> result;
    for (const Polyline& pl : ribb.lines()) {
        if (!pl.points.empty()) {

            Point_2 c_previous = pl.points.at(0);
            for (uint32_t i = 1; i < pl.points.size(); ++i) {

                const Point_2& c = pl.points.at(i);
                if (c_previous != c) {
                    result.emplace_back(c_previous, c);
                    c_previous = c;
                }
            }
        }
    }
    return result;
}

std::vector<Segment_2> SVGShapeToGrid::get_grid(const Ribbon& ribb) {

    std::vector<Segment_2> segs = SVGShapeToGrid::addToSegments(ribb);
    Arr_with_hist_2 arr;
    CGAL::insert(arr, segs.begin(), segs.end());

    return SVGShapeToGrid::get_grid(arr);

}

std::vector<CGAL::Polygon_with_holes_2<Kernel> > SVGShapeToGrid::get_polygons(const Ribbon& ribb) {

    std::vector<Segment_2> segs = SVGShapeToGrid::addToSegments(ribb);
    Arr_with_hist_2 arr;
    CGAL::insert(arr, segs.begin(), segs.end());
    markWindingRule(arr);
    return get_polygons(arr);
}

std::vector<Segment_2> SVGShapeToGrid::get_grid(Arr_with_hist_2& arr) {
    markWindingRule(arr);

    std::cout << " ARR number of edge " << arr.number_of_edges() << std::endl;
    auto res = get_polygons(arr);
    std::vector<Segment_2> segResult = createOffset(res, 1);

    return segResult;
}
} /* namespace laby */
