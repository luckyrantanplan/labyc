/*
 * SimplifyLines.h
 *
 *  Created on: Mar 29, 2018
 *      Author: florian
 */

#ifndef BASIC_SIMPLIFYLINES_H_
#define BASIC_SIMPLIFYLINES_H_

#include <array>
#include <cstddef>

#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/register/point.hpp>

// #include "Ribbon.h"

namespace laby {

struct IndexedPoint {
    IndexedPoint() = default;

    static auto fromCoordinates(const std::array<double, 2>& coordinates,
                                std::size_t pointIndex) -> IndexedPoint {
        IndexedPoint point;
        point.setX(coordinates[0]);
        point.setY(coordinates[1]);
        point.setIndex(pointIndex);
        return point;
    }

    [[nodiscard]] auto x() const -> double {
        return _x;
    }

    [[nodiscard]] auto y() const -> double {
        return _y;
    }

    [[nodiscard]] auto index() const -> std::size_t {
        return _index;
    }

    void setX(double xCoordinate) {
        _x = xCoordinate;
    }

    void setY(double yCoordinate) {
        _y = yCoordinate;
    }

    void setIndex(std::size_t pointIndex) {
        _index = pointIndex;
    }

  private:
    double _x = 0;
    double _y = 0;
    std::size_t _index = 0;
};
} /* namespace laby */

BOOST_GEOMETRY_REGISTER_POINT_2D_GET_SET(laby::IndexedPoint, double, cs::cartesian, x, y, setX,
                                         setY)

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
