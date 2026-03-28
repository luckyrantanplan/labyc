/*
 * Polyline.h
 *
 *  Created on: Sep 11, 2018
 *      Author: florian
 */

#ifndef POLYLINE_H_
#define POLYLINE_H_

#include <cstdint>
#include <CGAL/number_utils.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <vector>

#include "GeomData.h"
#include <CGAL/Polygon_2.h>

namespace laby {

class Polyline {

public:

    Polyline() = default;

    Polyline(const int32_t polyId, const std::vector<Point_2>& pts) : _id{polyId}, _points{pts} {}

    explicit Polyline(const int32_t polyId) : _id{polyId} {}

    explicit Polyline(const CGAL::Polygon_2<Kernel>& poly)
        : _points{poly.container()}, _closed{true} {}

    [[nodiscard]] auto empty() const -> bool { return _points.empty(); }

    [[nodiscard]] auto id() const -> int32_t { return _id; }

    auto setId(const int32_t polylineId) -> void { _id = polylineId; }

    [[nodiscard]] auto points() const -> const std::vector<Point_2>& { return _points; }

    auto points() -> std::vector<Point_2>& { return _points; }

    [[nodiscard]] auto isClosed() const -> bool { return _closed; }

    auto setClosed(const bool isClosed) -> void { _closed = isClosed; }

    [[nodiscard]] auto minPoint() const -> const Point_2& { return _minPoint; }

    void reverse();

    void print(std::ostream& outputStream) const {
        outputStream << " id " << _id << " points " << _points;
    }

    void computeMinLexi() {
        _minPoint = *std::min_element(_points.begin(), _points.end(),
                                      [](const Point_2& leftPoint,
                                         const Point_2& rightPoint) {
                                          return leftPoint < rightPoint;
                                      });
    }

    [[nodiscard]] auto totalLength() const -> double {
        double sqdist = 0;
        for (std::size_t i = 1; i < _points.size(); ++i) {
            sqdist +=
                sqrt(CGAL::to_double((_points.at(i) - _points.at(i - 1)).squared_length()));
        }
        return sqdist;
    }

    void removeConsecutiveDuplicatePoints(double epsilon = 0.0);

    void simplify(double distance);

private:
    int32_t _id = 0;
    std::vector<Point_2> _points;
    bool _closed = false;
    Point_2 _minPoint;
};

} /* namespace laby */

#endif /* POLYLINE_H_ */
