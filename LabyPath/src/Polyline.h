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

struct Polyline {

    Polyline() = default;

    Polyline(const int32_t polyId, const std::vector<Point_2>& pts) : id{polyId}, points{pts} {}

    explicit Polyline(const int32_t polyId) : id{polyId} {}

    explicit Polyline(const CGAL::Polygon_2<Kernel>& poly) : points{poly.container()}, closed{true} {}

    [[nodiscard]] bool empty() const { return points.empty(); }

    void reverse();

    void print(std::ostream& os) const { os << " id " << id << " points " << points; }

    void computeMinLexi() {
        min_point = *std::min_element(points.begin(), points.end(), [](const Point_2& a, const Point_2& b) { return a < b; });
    }

    [[nodiscard]] double totalLength() const {
        double sqdist = 0;
        for (std::size_t i = 1; i < points.size(); ++i) {
            sqdist += sqrt(CGAL::to_double((points.at(i) - points.at(i - 1)).squared_length()));
        }
        return sqdist;
    }

    void removeConsecutiveDuplicatePoints(double epsilon = 0.0);

    void simplify(double distance);

    int32_t id = 0;              // NOLINT(misc-non-private-member-variables-in-classes)
    std::vector<Point_2> points; // NOLINT(misc-non-private-member-variables-in-classes)
    bool closed = false;         // NOLINT(misc-non-private-member-variables-in-classes)
    Point_2 min_point;           // NOLINT(misc-non-private-member-variables-in-classes)
};

} /* namespace laby */

#endif /* POLYLINE_H_ */
