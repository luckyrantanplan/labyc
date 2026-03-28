/*
 * LinearGradient.cpp
 *
 *  Created on: Apr 5, 2018
 *      Author: florian
 */

#include "LinearGradient.h"
#include "GeomData.h"

#include <CGAL/Kernel/global_functions_2.h>
#include <CGAL/Vector_2.h>
#include <CGAL/number_utils.h>

#include <utility>


namespace laby::basic {

LinearGradient::LinearGradient(Point_2  p1, const double thickness1, const Point_2& p2, const double thickness2) :
        _thickness1 { thickness1 }, _thickness2 { thickness2 }, _origin {std::move( p1 )} {

    _vec = p2 - _origin;
    _sq_length = CGAL::to_double(_vec.squared_length());

}

auto LinearGradient::f(double t, double thickness1, double thickness2) -> double {
    t = t * t;
    t = t * t;
    return (thickness1 * (1 - t) + thickness2 * t);
}

auto LinearGradient::thickness(const Point_2& p) -> double {

    CGAL::Vector_2<Kernel> const vec = p - _origin;
    ;
    double const t = CGAL::to_double(vec * _vec) / _sq_length;

    if (t <= 0) {
        return _thickness1;
    }

    if (t >= 1) {
        return _thickness2;
    }

    if (_thickness1 > _thickness2) {
        return f(1 - t, _thickness2, _thickness1);
    }
    return f(t, _thickness1, _thickness2);
}

} // namespace laby::basic

