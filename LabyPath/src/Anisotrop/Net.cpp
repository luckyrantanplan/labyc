/*
 * Net.cpp
 *
 *  Created on: Feb 8, 2018
 *      Author: florian
 */

#include "Net.h"

#include "../basic/LinearGradient.h"
#include <CGAL/Arr_dcel_base.h>
#include <CGAL/Point_2.h>
#include <CGAL/number_utils.h>

namespace laby::aniso {

auto Net::extractPins(const std::vector<Net>& nets) -> std::vector<std::complex<double>> {
    std::vector<std::complex<double>> result;
    for (const Net& net : nets) {
        {
            const Point_2& point = net.source().vertex().point();
            result.emplace_back(CGAL::to_double(point.x()), CGAL::to_double(point.y()));
        }
        {
            const Point_2& point = net.target().vertex().point();
            result.emplace_back(CGAL::to_double(point.x()), CGAL::to_double(point.y()));
        }
    }
    return result;
}
auto Net::gradient() const -> laby::basic::LinearGradient {
    return {_source.vertex().point(), _source.thickness(), _target.vertex().point(),
            _target.thickness()};
}
} // namespace laby::aniso
