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

//#include "Ribbon.h"

namespace laby {

struct Indexed_Point {
    Indexed_Point(double ix, double iy, std::size_t i) :
            x(ix), y(iy), index(i) {
    }
    Indexed_Point() {
    }
    ;
    double x = 0;
    double y = 0;
    std::size_t index = 0;

};
} /* namespace laby */

BOOST_GEOMETRY_REGISTER_POINT_2D(laby::Indexed_Point, double, cs::cartesian, x, y)

namespace laby {
class SimplifyLines {
public:

    typedef boost::geometry::model::d2::point_xy<double> xy;

    typedef boost::geometry::model::linestring<xy> LineString;

    static LineString decimate(LineString& line, double dist);

    typedef boost::geometry::model::linestring<Indexed_Point> LineStringIndexed;

    static LineStringIndexed decimateIndex(LineStringIndexed& line, double dist);

};

} /* namespace laby */

#endif /* BASIC_SIMPLIFYLINES_H_ */
