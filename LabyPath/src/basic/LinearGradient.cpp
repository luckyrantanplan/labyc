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

LinearGradient::LinearGradient(Point_2 startPoint, const double thickness1,
                               const Point_2& endPoint, const double thickness2) :
        _thickness1 { thickness1 }, _thickness2 { thickness2 }, _origin {std::move(startPoint)} {

    _vec = endPoint - _origin;
    _sq_length = CGAL::to_double(_vec.squared_length());

}

auto LinearGradient::f(double position, double startThickness, double endThickness) -> double {
    position = position * position;
    position = position * position;
    return (startThickness * (1 - position) + endThickness * position);
}

auto LinearGradient::thickness(const Point_2& point) -> double {

    CGAL::Vector_2<Kernel> const offsetVector = point - _origin;
    double const position = CGAL::to_double(offsetVector * _vec) / _sq_length;

    if (position <= 0) {
        return _thickness1;
    }

    if (position >= 1) {
        return _thickness2;
    }

    if (_thickness1 > _thickness2) {
        return f(1 - position, _thickness2, _thickness1);
    }
    return f(position, _thickness1, _thickness2);
}

} // namespace laby::basic

