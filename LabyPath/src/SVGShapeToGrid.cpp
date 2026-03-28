/*
 * SVGShapeToGrid.cpp
 *
 *  Created on: Jun 21, 2018
 *      Author: florian
 */

#include "SVGShapeToGrid.h"

#include <CGAL/Arr_dcel_base.h>
#include <CGAL/Arr_geometry_traits/Consolidated_curve_data_aux.h>
#include <CGAL/Arrangement_on_surface_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/Vector_2.h>
#include <CGAL/Point_2.h>
#include <CGAL/Arrangement_on_surface_with_history_2.h>
#include <CGAL/create_offset_polygons_2.h>
#include <CGAL/create_straight_skeleton_2.h>
#include <CGAL/Direction_2.h>
#include <CGAL/Iterator_transform.h>
#include <CGAL/Kernel/global_functions_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_set_2.h>
#include <cstdint>
#include <CGAL/number_utils.h>
#include <iostream>
#include <iterator>
#include <queue>
#include <vector>

#include "GeomData.h"
#include "Ribbon.h"
#include "Polyline.h"
#include "basic/RangeHelper.h"

namespace laby {

void SVGShapeToGrid::markFace(Halfedge& he, Face& parent_face, std::queue<Face*>& q) {
    FaceNonZeroData& data = he.face()->data();
    if (!data._walked) {
        int32_t windingNumber = parent_face.data()._winding_number;
        const Halfedge::X_monotone_curve& curve = he.curve();
        const CGAL::_Unique_list<Segment_2 *> curveData = curve.data();

        for (const Segment_2 * ocit : RangeHelper::make(curveData.begin(), curveData.end())) {
            const Segment_2& seg = *ocit;

            if (CGAL::Direction_2<Kernel>(seg) == CGAL::Direction_2<Kernel>(he.target()->point() - he.source()->point())) {
                ++windingNumber;
            } else {
                --windingNumber;
            }

        }
        data._walked = true;
        data._winding_number = windingNumber;
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
        std::cout << " outP.is_simple() " << outP.is_simple() << '\n';
        for (const Kernel::Point_2& p : outP.container()) {
            std::cout << p << "\n";
        }
        std::cout << "\n";
    }
}

auto SVGShapeToGrid::getPolygons(Arr_with_hist_2& arr) -> std::vector<CGAL::Polygon_with_holes_2<Kernel> > {
    CGAL::Polygon_set_2<Kernel> polySet;
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
            CGAL::Polygon_with_holes_2<Kernel> const polyHole(outP, holesP.begin(), holesP.end());

            debugPoly(outP);

            polySet.join(polyHole);
        }
    }

    std::vector<CGAL::Polygon_2<Kernel> > patches;

    const auto& arrSet = polySet.arrangement();
    CGAL::Vector_2<Kernel> const vX(0.1, 0);
    CGAL::Vector_2<Kernel> const vY(0, 0.1);
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
        polySet.join(lp);
    }

    std::vector<CGAL::Polygon_with_holes_2<Kernel> > res;
    std::cout << "The result contains " << polySet.number_of_polygons_with_holes() << " components:" << '\n';
    polySet.polygons_with_holes(std::back_inserter(res));

    return res;
}

auto SVGShapeToGrid::createOffset(std::vector<CGAL::Polygon_with_holes_2<Kernel> >& res, const double l) -> std::vector<Segment_2> {
    std::cout << "SVGShapeToGrid::createOffset " << res.size() << '\n';
    SsPtr const iss = CGAL::create_interior_straight_skeleton_2(res.front());
    std::cout << "first step OK " << '\n';
    std::vector<Segment_2> segResult;
    for (double ll = 0.01; ll < 10.; ll += l) {

        auto offsetPolygons = CGAL::create_offset_polygons_2<CGAL::Polygon_2<InexactK> >(ll, *iss);
        for (const auto& polySp : offsetPolygons) {
            const auto& poly = *polySp;
            for (auto eit = poly.edges_begin(); eit != poly.edges_end(); ++eit) {
                auto src = *eit;
                segResult.emplace_back(
                    Point_2(CGAL::to_double(src.source().x()), CGAL::to_double(src.source().y())),
                    Point_2(CGAL::to_double(src.target().x()), CGAL::to_double(src.target().y()))
                );
            }
        }
    }
    std::cout << "SVGShapeToGrid::createOffset end" << '\n';
    return segResult;
}

auto SVGShapeToGrid::addToSegments(const Ribbon& ribb) -> std::vector<Segment_2> {
    std::vector<Segment_2> result;
    for (const Polyline& pl : ribb.lines()) {
        if (!pl.points.empty()) {

            Point_2 cPrevious = pl.points.at(0);
            for (uint32_t i = 1; i < pl.points.size(); ++i) {

                const Point_2& c = pl.points.at(i);
                if (cPrevious != c) {
                    result.emplace_back(cPrevious, c);
                    cPrevious = c;
                }
            }
        }
    }
    return result;
}

auto SVGShapeToGrid::getGrid(const Ribbon& ribb) -> std::vector<Segment_2> {

    std::vector<Segment_2> segs = SVGShapeToGrid::addToSegments(ribb);
    Arr_with_hist_2 arr;
    CGAL::insert(arr, segs.begin(), segs.end());

    return SVGShapeToGrid::getGrid(arr);

}

auto SVGShapeToGrid::getPolygons(const Ribbon& ribb) -> std::vector<CGAL::Polygon_with_holes_2<Kernel> > {

    std::vector<Segment_2> segs = SVGShapeToGrid::addToSegments(ribb);
    Arr_with_hist_2 arr;
    CGAL::insert(arr, segs.begin(), segs.end());
    markWindingRule(arr);
    return getPolygons(arr);
}

auto SVGShapeToGrid::getGrid(Arr_with_hist_2& arr) -> std::vector<Segment_2> {
    markWindingRule(arr);

    std::cout << " ARR number of edge " << arr.number_of_edges() << '\n';
    auto res = getPolygons(arr);
    std::vector<Segment_2> segResult = createOffset(res, 1);

    return segResult;
}
} /* namespace laby */
