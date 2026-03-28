/*
 * PolygonTools.cpp
 *
 *  Created on: Mar 8, 2018
 *      Author: florian
 */

#include "PolygonTools.h"
#include "GeomData.h"
#include "basic/RangeHelper.h"

#include <CGAL/enum.h>
#include <CGAL/Kernel/global_functions_2.h>
#include <CGAL/Lazy_kernel.h>
#include <CGAL/number_utils.h>
#include <CGAL/Vector_2.h>
#include <cmath>
#include <iostream>
#include <ostream>

namespace laby {

auto PolygonTools::makeTrapeze(const Point_2& a, const Point_2& b, const double& thickness1, const double& thickness2) -> Linear_polygon {

    Linear_polygon poly;
    makeTrapeze(poly, a, b, thickness1, thickness2);

    return poly;
}

void PolygonTools::makeTrapeze(Linear_polygon& poly, const Point_2& a, const Point_2& b, const double& thickness1, const double& thickness2) {
    CGAL::Vector_2<Kernel> const vec(a, b);
    CGAL::Vector_2<Kernel> const perp = vec.perpendicular(CGAL::LEFT_TURN);

    double const f = 0.5 / std::sqrt(CGAL::to_double(perp.squared_length()));

    double const length1 = thickness1 * f;
    CGAL::Vector_2<Kernel> const perp1 = perp * length1;

    poly.push_back(a + perp1);
    poly.push_back(a - perp1);

    double const length2 = thickness2 * f;
    CGAL::Vector_2<Kernel> const perp2 = perp * length2;
    poly.push_back(b - perp2);
    poly.push_back(b + perp2);

}

void PolygonTools::insertPointPolygon(const Point_2& a2, const Point_2& a3, const Point_2& b0, const Point_2& b1, Linear_polygon& p1) {
    Kernel::Orientation const orientation = CGAL::orientation(a2, a3, b0);
    if (orientation == CGAL::RIGHT_TURN) {
        if (CGAL::compare_squared_distance(a3, b0, 0) == CGAL::LARGER) {
            p1.insert(p1.vertices_begin() + 3, b0);
        }
    } else {
        if (orientation == CGAL::LEFT_TURN) {
            if (CGAL::compare_squared_distance(a2, b1, 0) == CGAL::LARGER) {
                p1.insert(p1.vertices_begin() + 3, b1);
            }
        }
    }
}

void PolygonTools::extendPolygon(Linear_polygon& p1, const Linear_polygon& p2) {
    const Point_2& b0 = p2.vertex(0);
    const Point_2& b1 = p2.vertex(1);

    const Point_2& a3 = p1.vertex(3);
    const Point_2& a2 = p1.vertex(2);

    insertPointPolygon(a2, a3, b0, b1, p1);

}

auto PolygonTools::getSegmentContainingPoint(const Linear_polygon& p1, const Point_2& center) -> const Linear_polygon::Segment_2 {
    for (const Linear_polygon::Segment_2& seg : RangeHelper::make(p1.edges_begin(), p1.edges_end())) {
        if (seg.has_on(center)) {
            return seg;
        }
    }
    std::cout << "ERROR seg of poly " << p1 << " does not have center point " << center << '\n';
    return Linear_polygon::Segment_2();
}



auto PolygonTools::createJoinTriangle(const Linear_polygon& p1, const Linear_polygon& p2, const Point_2& center) -> Linear_polygon {
    Linear_polygon lp;

    const Linear_polygon::Segment_2 seg1 = getSegmentContainingPoint(p1, center);
    const Linear_polygon::Segment_2 seg2 = getSegmentContainingPoint(p2, center);

    Kernel::Orientation const orientation = CGAL::orientation(seg1.source(), seg2.source(), seg2.target());

    if (orientation == CGAL::LEFT_TURN) {

        lp.push_back(seg1.target());
        lp.push_back(center);
        lp.push_back(seg2.source());

    } else if (orientation == CGAL::RIGHT_TURN) {
        lp.push_back(seg2.target());
        lp.push_back(center);
        lp.push_back(seg1.source());
    }

    return lp;
}

} /* namespace laby */
