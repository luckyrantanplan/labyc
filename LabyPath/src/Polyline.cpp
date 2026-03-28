/*
 * Polyline.cpp
 *
 *  Created on: Sep 11, 2018
 *      Author: florian
 */

#include "Polyline.h"

#include <CGAL/number_utils.h>
#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

#include "GeomData.h"
#include "basic/SimplifyLines.h"

namespace laby {

void Polyline::reverse() {
    std::reverse(_points.begin(), _points.end());
}

void Polyline::removeConsecutiveDuplicatePoints(double epsilon) {
    if (_points.size() < 2) {
        return;
    }

    std::vector<Point_2> result;
    result.reserve(_points.size());
    result.emplace_back(_points.front());
    const double sqEpsilon = epsilon * epsilon;
    for (std::size_t i = 1; i < _points.size(); ++i) {
        if (CGAL::to_double((_points.at(i) - result.back()).squared_length()) > sqEpsilon) {
            result.emplace_back(_points.at(i));
        }
    }
    _points = std::move(result);
}

void Polyline::simplify(double distance) {

    if (_points.size() > 2) {
        SimplifyLines::LineStringIndexed lineString;
        for (std::size_t i = 0; i < _points.size(); ++i) {
            const Point_2& point = _points.at(i);
            lineString.emplace_back(IndexedPoint::fromCoordinates(
                {CGAL::to_double(point.x()), CGAL::to_double(point.y())}, i));
        }
        const SimplifyLines::LineStringIndexed simpleLine =
            SimplifyLines::decimateIndex(lineString, distance);
        if (simpleLine.size() < lineString.size()) {
            std::vector<Point_2> result;
            result.reserve(simpleLine.size());
            for (const IndexedPoint& offset : simpleLine) {

                result.emplace_back(_points.at(offset.index()));
            }
            _points = result;
        }
    }
}
} /* namespace laby */
