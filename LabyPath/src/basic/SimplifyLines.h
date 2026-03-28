/*
 * SimplifyLines.h
 *
 *  Created on: Mar 29, 2018
 *      Author: florian
 */

#ifndef BASIC_SIMPLIFYLINES_H_
#define BASIC_SIMPLIFYLINES_H_

#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/register/point.hpp>

// #include "Ribbon.h"

namespace laby {

struct IndexedPoint {
    IndexedPoint(double ix, double iy, std::size_t i) : x(ix), y(iy), index(i) {}
    IndexedPoint() = default;

    double x = 0;
    double y = 0;
    std::size_t index = 0;
};
} /* namespace laby */

BOOST_GEOMETRY_REGISTER_POINT_2D(laby::IndexedPoint, double, cs::cartesian, x, y)

namespace laby {
class SimplifyLines {
  public:
    using xy = boost::geometry::model::d2::point_xy<double>;

    using LineString = boost::geometry::model::linestring<xy>;

    static auto decimate(LineString& line, double dist) -> LineString;

    using LineStringIndexed = boost::geometry::model::linestring<IndexedPoint>;

    static auto decimateIndex(LineStringIndexed& line, double dist) -> LineStringIndexed;
};

} /* namespace laby */

#endif /* BASIC_SIMPLIFYLINES_H_ */
