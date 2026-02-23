/*
 * LinearGradient.cpp
 *
 *  Created on: Apr 5, 2018
 *      Author: florian
 */

#include "LinearGradient.h"

#include <CGAL/Kernel/global_functions_2.h>
#include <CGAL/number_utils.h>

namespace laby {
namespace basic {

LinearGradient::LinearGradient(const Point_2& p1, const double thickness1, const Point_2& p2, const double thickness2) :
        _thickness1 { thickness1 }, _thickness2 { thickness2 }, origin { p1 } {

    _vec = p2 - origin;
    _sq_length = CGAL::to_double(_vec.squared_length());

}

double LinearGradient::f(double t, double thickness1, double thickness2) {
    t = t * t;
    t = t * t;
    return (thickness1 * (1 - t) + thickness2 * t);
}

double LinearGradient::thickness(const Point_2& p) {

    CGAL::Vector_2<Kernel> vec = p - origin;
    ;
    double t = CGAL::to_double(vec * _vec) / _sq_length;

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

} /* namespace basic */
} /* namespace laby */
