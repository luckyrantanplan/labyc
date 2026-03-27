/*
 * Polyline.cpp
 *
 *  Created on: Sep 11, 2018
 *      Author: florian
 */

#include "Polyline.h"

#include <algorithm>
#include <cstddef>
#include <CGAL/number_utils.h>
#include <utility>
#include <vector>

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
        if (CGAL::to_double((points.at(i) - result.back()).squared_length()) > sqEpsilon) {
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
            lineString.emplace_back(IndexedPoint(CGAL::to_double(point.x()), CGAL::to_double(point.y()), i));
        }
        const SimplifyLines::LineStringIndexed simpleLine = SimplifyLines::decimateIndex(lineString, distance);
        if (simpleLine.size() < lineString.size()) {
            std::vector<Point_2> result;
            result.reserve(simpleLine.size());
            for (const IndexedPoint& offset : simpleLine) {

                result.emplace_back(points.at(offset.index));
            }
            points = result;
        }
    }
}
} /* namespace laby */
