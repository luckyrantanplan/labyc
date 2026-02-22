/*
 * Net.cpp
 *
 *  Created on: Feb 8, 2018
 *      Author: florian
 */

#include "Net.h"

#include <CGAL/Arr_dcel_base.h>
#include <CGAL/number_utils.h>
#include <CGAL/Point_2.h>
#include "../basic/LinearGradient.h"

namespace laby {
namespace aniso {

std::vector<std::complex<double> > Net::extractPins(const std::vector<Net>& nets) {
    std::vector<std::complex<double> > result;
    for (const Net& n : nets) {
        {
            const Point_2& p = n.source().vertex().point();
            result.emplace_back(CGAL::to_double(p.x()), CGAL::to_double(p.y()));
        }
        {
            const Point_2& p = n.target().vertex().point();
            result.emplace_back(CGAL::to_double(p.x()), CGAL::to_double(p.y()));
        }
    }
    return result;
}
laby::basic::LinearGradient Net::gradient() const {
    return basic::LinearGradient(_source.vertex().point(), _source.thickness(), _target.vertex().point(), _target.thickness());
}
}/* namespace aniso */
}/* namespace laby */
