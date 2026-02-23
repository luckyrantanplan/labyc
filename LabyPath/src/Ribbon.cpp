/*
 * Ribbon.cpp
 *
 *  Created on: Feb 26, 2018
 *      Author: florian
 */

#include "Ribbon.h"

#include <cstdint>
#include <boost/geometry/geometries/point_xy.hpp>
#include <CGAL/Arr_dcel_base.h>
#include <CGAL/Arr_geometry_traits/Curve_data_aux.h>
#include <CGAL/Arrangement_2/Arrangement_on_surface_2_global.h>
#include <CGAL/Arrangement_on_surface_2.h>
#include <CGAL/Compact_container.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/enum.h>
#include <CGAL/Kernel/global_functions_2.h>
#include <CGAL/number_utils.h>
#include <CGAL/Point_2.h>
#include <CGAL/Point_set_2.h>
#include <CGAL/Triangulation_vertex_base_2.h>
#include "basic/EasyProfilerCompat.h"
#include <map>
#include <queue>
#include <utility>
#include "Rendering/GraphicRendering.h"
#include "basic/SimplifyLines.h"

namespace laby {

std::vector<Point_2> Ribbon::get_Points() const {
    std::vector<Point_2> pointsVect;
    for (const Polyline& pl : _lines) {
        if (!pl.points.empty()) {

            Point_2 c_previous = pl.points.at(0);
            for (const Point_2& c : pl.points) {
                pointsVect.emplace_back(c);
            }
        }
    }
    return pointsVect;
}

void Ribbon::addToSegments(std::vector<Segment_info_2>& listSeg) const {
    for (const Polyline& pl : _lines) {
        if (!pl.points.empty()) {

            Point_2 c_previous = pl.points.at(0);
            for (std::size_t i = 1; i < pl.points.size(); ++i) {
                const Point_2& c = pl.points.at(i);
                Segment_2 s { c_previous, c };
                std::size_t coordinate = static_cast<std::size_t>(pl.number);
                listSeg.push_back(Segment_info_2(s, EdgeInfo { _fill_color, coordinate }));
                c_previous = c;
            }
        }
    }
}

Arrangement_2 Ribbon::createArr(const Ribbon & r1, const Ribbon & r2) {
    Arrangement_2 arr;
    appendToArr(r1, r2, arr);
    return arr;
}

Arrangement_2 Ribbon::createArr(const std::vector<Ribbon> & ribList) {
    Arrangement_2 arr;
    std::vector<Segment_info_2> listSeg;
    for (const Ribbon& rib : ribList) {
        rib.addToSegments(listSeg);
    }
    CGAL::insert(arr, listSeg.begin(), listSeg.end());
    return arr;
}

std::vector<Kernel::Segment_2> Ribbon::get_segments() const {
    std::vector<Kernel::Segment_2> listSeg;
    for (const Polyline& pl : _lines) {
        if (!pl.points.empty()) {

            Point_2 c_previous = pl.points.at(0);
            for (std::size_t i = 1; i < pl.points.size(); ++i) {
                const Point_2& c = pl.points.at(i);
                if (c_previous == c) {
                    std::cout << "segment is degenerate !!" << std::endl;
                } else {

                    listSeg.emplace_back(c_previous, c);
                }
                c_previous = c;
            }
        }
    }
    return listSeg;
}

void Ribbon::appendToArr(const Ribbon & r1, const Ribbon & r2, Arrangement_2& arr) {
    EASY_FUNCTION();
    std::vector<Segment_info_2> listSeg;
    r1.addToSegments(listSeg);
    r2.addToSegments(listSeg);
    CGAL::insert(arr, listSeg.begin(), listSeg.end());

}

void Ribbon::reverse() {
    for (Polyline& pl : lines()) {
        pl.reverse();

    }
}

Arrangement_2 Ribbon::createArr() const {
    EASY_FUNCTION();
    Arrangement_2 arr;
    std::vector<Segment_info_2> listSeg;
    addToSegments(listSeg);
    CGAL::insert(arr, listSeg.begin(), listSeg.end());
    return arr;
}

std::vector<std::size_t> Ribbon::middleOrder(std::size_t min, std::size_t max) const {
    std::queue<Ele> queue;
    std::vector<std::size_t> result;
    result.reserve(max - min);
    queue.push(Ele(min, max));

    while (!queue.empty()) {

        Ele e = queue.front();
        queue.pop();
        std::size_t middle = (e.min + e.max) / 2;
        result.push_back(middle);
        if (middle != e.min) {
            queue.push( { e.min, middle - 1 });
        }
        if (middle != e.max) {
            queue.push( { middle + 1, e.max });
        }
    }
    return result;
}

bool Ribbon::isLongEnough(const std::vector<Point_2>& coarse, double thickness) const {
    //TODO : use polyline instead of vector<Point_2>

    if (coarse.size() < 2) {
        return false;
    }
    Polyline pl(0, coarse);
    return pl.total_length() > thickness;
}

void Ribbon::order_lines() {
    for (Polyline& polyline : _lines) {
        polyline.compute_min_lexi();
    }
    std::sort(std::begin(_lines), std::end(_lines), [](const Polyline& a, const Polyline& b) {
        return a.min_point < b.min_point;
    });

    for (std::size_t i = 0; i < _lines.size(); ++i) {
        _lines.at(i).number = static_cast<int32_t>(i);
    }
}

Ribbon Ribbon::give_space(const double& space, const double& subdivision_factor, const double& minimal_length) const {

    Ribbon result = subdived(space / subdivision_factor);

    result.order_lines();
    return result.subRibbon(space, minimal_length);
}

Ribbon Ribbon::subdived(const double& thickness) const {
    Ribbon result(_fill_color);
    for (const Polyline& polyline : _lines) {
        result._lines.emplace_back(polyline.number);
        std::vector<Kernel::Point_2>& pt_list = result._lines.back().points;
        pt_list.emplace_back(polyline.points.front());
        for (std::size_t i = 1; i < polyline.points.size(); ++i) {
            const Point_2& a = polyline.points.at(i - 1);
            const Point_2& b = polyline.points.at(i);
            double l = sqrt(CGAL::to_double(CGAL::squared_distance(a, b)));
            if (l > thickness) {
                double weight = l / thickness;

                for (double j = 1; j < weight; ++j) {
                    const Kernel::Point_2 test = CGAL::barycenter(b, j / weight, a);
                    pt_list.emplace_back(test);
                }
            }
            pt_list.emplace_back(b);

        }
    }
    return result;
}
Ribbon Ribbon::subRibbon(const double& space, const double& minimal_length) const {
    EASY_FUNCTION();

    typedef CGAL::Point_set_2<Kernel>::Vertex_handle Vertex_handlePS;

    CGAL::Point_set_2<Kernel> pointSet;

    Ribbon coarseq(_fill_color);

    std::map<int32_t, std::vector<Polyline>> flattenRibbonMap;

    for (const Polyline& polyline : _lines) {
        std::vector<Polyline>& p = flattenRibbonMap[polyline.number];
        p.push_back(polyline);
    }

    std::vector<std::vector<Polyline>> flattenRibbon;
    flattenRibbon.reserve(flattenRibbonMap.size());

    for (auto& pair : flattenRibbonMap) {

        flattenRibbon.push_back(pair.second);
    }

    std::vector<std::size_t> order = middleOrder(0lu, flattenRibbon.size() - 1);
    for (std::size_t i : order) {

        for (const Polyline& polyline : flattenRibbon.at(i)) {
            std::vector<Point_2> coarse;

            for (const Point_2& p : polyline.points) {
                Vertex_handlePS vh = pointSet.nearest_neighbor(p);

                if (vh != nullptr and //
                        CGAL::compare_squared_distance(vh->point(), p, space * space) != CGAL::LARGER) {

                    if (isLongEnough(coarse, minimal_length)) {
                        coarseq.lines().push_back(Polyline { polyline.number, coarse });
                        pointSet.insert(coarse.begin(), coarse.end());
                    }
                    coarse.clear();
                } else {
                    coarse.push_back(p);
                }
            }
            if (isLongEnough(coarse, minimal_length)) {

                coarseq.lines().push_back(Polyline { polyline.number, coarse });
                pointSet.insert(coarse.begin(), coarse.end());
            }
        }
    }

    return coarseq;
}

std::vector<Ribbon> Ribbon::splitRibbon(const double thickness, const int octave) {
    std::vector<Ribbon> stackRibbon;
    stackRibbon.push_back(*this);
    double p = 1;
    for (int i = 0; i < octave; ++i) {
        stackRibbon.push_back(stackRibbon.back().subRibbon(p * thickness, p * thickness));
        p *= 2;
    }
    return stackRibbon;
}

void Ribbon::simplify(const double dist) {
    for (Polyline& pl : lines()) {
        pl.simplify(dist);

    }
}

Ribbon Ribbon::createRibbonOfEdge(const Arrangement_2& arr, const double simplification) {
// Print the arrangement edges.
    EASY_FUNCTION();

//init
    for (const Halfedge& eit : RangeHelper::make(arr.edges_begin(), arr.edges_end())) {
        const Segment_info_2& curve = eit.curve();
        curve.data().setVisit(-1);
    }

    Ribbon ribbon;

    for (const Vertex& v : RangeHelper::make(arr.vertices_begin(), arr.vertices_end())) {
        if (!v.is_isolated() and v.degree() != 2) {
            for (const Halfedge& eit : RangeHelper::make(v.incident_halfedges())) {
                if (eit.curve().data().getVisit() == -1) {
                    ribbon.lines().emplace_back();
                    Polyline& poly = ribbon.lines().back();

                    for (const Halfedge& he : RangeHelper::make(eit.twin()->ccb())) {
                        poly.points.emplace_back(he.source()->point());

                        if (he.curve().data().getVisit() == 1) {
                            break;
                        }

                        if (he.target()->degree() != 2) {
                            he.curve().data().setVisit(1);
                            poly.points.emplace_back(he.target()->point());
                            break;
                        }
                        he.curve().data().setVisit(1);
                    }

                }

            }
        }

    }
//loop (only degree 2 )
    for (const Halfedge& eit : RangeHelper::make(arr.edges_begin(), arr.edges_end())) {

        if (eit.curve().data().getVisit() != 1) {

            ribbon.lines().emplace_back();
            Polyline& poly = ribbon.lines().back();
            bool closed = true;

            for (const Halfedge& he : RangeHelper::make(eit.twin()->ccb())) {
                poly.points.emplace_back(he.source()->point());
                if (he.curve().data().getVisit() == 1) {
                    closed = false;
                    break;
                }
                he.curve().data().setVisit(1);
            }
            if (closed) {
                poly.points.emplace_back(eit.target()->point());
                poly.closed = true;
            }

        }
    }
// to do : add this parameter in the configuration
    if (simplification > 0) {
        ribbon.simplify(simplification);
    }
    return ribbon;

}

} /* namespace laby */
