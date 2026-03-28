/*
 * SimplifyLines.cpp
 *
 *  Created on: Mar 29, 2018
 *      Author: florian
 */

#include "SimplifyLines.h"
#include <boost/geometry.hpp>

#include <boost/geometry/algorithms/simplify.hpp>

namespace laby {

SimplifyLines::LineString SimplifyLines::decimate(LineString& line, double dist) {

    LineString simplified;
    boost::geometry::simplify(line, simplified, dist);

    return simplified;
}

SimplifyLines::LineStringIndexed SimplifyLines::decimateIndex(LineStringIndexed& line,
                                                              double dist) {

    LineStringIndexed simplified;
    boost::geometry::simplify(line, simplified, dist);

    return simplified;
}
} /* namespace laby */
