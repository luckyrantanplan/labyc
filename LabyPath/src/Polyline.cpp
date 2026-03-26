/*
 * Polyline.cpp
 *
 *  Created on: Sep 11, 2018
 *      Author: florian
 */

#include "Polyline.h"

#include <boost/geometry/geometries/point_xy.hpp>
#include <CGAL/Arr_dcel_base.h>
#include <CGAL/Arr_geometry_traits/Curve_data_aux.h>
#include <CGAL/number_utils.h>
#include <CGAL/Point_2.h>
#include "GeomData.h"
#include "basic/SimplifyLines.h"

namespace laby {

void Polyline::reverse() {
    std::reverse(points.begin(), points.end());
}

void Polyline::removeConsecutiveDuplicatePoints(double epsilon) {
    if (points.size() < 2) {
        return;
    }

    std::vector<Point_2> result;
    result.reserve(points.size());
    result.emplace_back(points.front());
    const double sqEpsilon = epsilon * epsilon;
    for (std::size_t i = 1; i < points.size(); ++i) {
        if (CGAL::compare_squared_distance(points.at(i), result.back(), sqEpsilon) == CGAL::LARGER) {
            result.emplace_back(points.at(i));
        }
    }
    points = std::move(result);
}

void Polyline::simplify(double distance) {

    if (points.size() > 2) {
        SimplifyLines::LineStringIndexed lineString;
        for (std::size_t i = 0; i < points.size(); ++i) {
            const Point_2& point = points.at(i);
            lineString.emplace_back(Indexed_Point(CGAL::to_double(point.x()), CGAL::to_double(point.y()), i));
        }
        SimplifyLines::LineStringIndexed simpleLine = SimplifyLines::decimateIndex(lineString, distance);
        if (simpleLine.size() < lineString.size()) {
            std::vector<Point_2> result;
            result.reserve(simpleLine.size());
            for (const Indexed_Point& offset : simpleLine) {

                result.emplace_back(points.at(offset.index));
            }
            points = result;
        }
    }

}
} /* namespace laby */
