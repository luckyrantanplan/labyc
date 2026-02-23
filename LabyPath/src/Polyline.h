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

    Polyline() {
    }

    Polyline(const int32_t num, const std::vector<Point_2>& pts) :
            number { num }, points { pts } {
    }

    explicit Polyline(const int32_t num) :
            number { num } {
    }

    explicit Polyline(const CGAL::Polygon_2<Kernel>& poly) :
            number { 0 }, points { } {

        points = poly.container();
        closed = true;
    }

    bool empty() const {
        return points.empty();
    }

    void reverse();

    void print(std::ostream& os) const {

        os << " number " << number << " points " << points;

    }

    void compute_min_lexi() {
        min_point = *std::min_element(points.begin(), points.end(), [](const Point_2& a,const Point_2& b) {
            return a<b;
        });
    }

    double total_length() const {
        double sqdist = 0;
        for (std::size_t i = 1; i < points.size(); ++i) {
            sqdist += sqrt(CGAL::to_double((points.at(i) - points.at(i - 1)).squared_length()));
        }
        return sqdist;
    }

    void simplify(double distance);

    int32_t number = 0;
    std::vector<Point_2> points;
    bool closed = false;
    Point_2 min_point;
};

} /* namespace laby */

#endif /* POLYLINE_H_ */
