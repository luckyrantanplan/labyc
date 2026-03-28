/*
 * SimplifyLines.cpp
 *
 *  Created on: Mar 29, 2018
 *      Author: florian
 */

#include "SimplifyLines.h"

#include <boost/geometry/algorithms/simplify.hpp>

namespace laby {

auto SimplifyLines::decimate(LineString& line, double dist) -> SimplifyLines::LineString {

    LineString simplified;
    boost::geometry::simplify(line, simplified, dist);

    return simplified;
}

auto SimplifyLines::decimateIndex(LineStringIndexed& line,
                                                              double dist) -> SimplifyLines::LineStringIndexed {

    LineStringIndexed simplified;
    boost::geometry::simplify(line, simplified, dist);

    return simplified;
}
} /* namespace laby */
