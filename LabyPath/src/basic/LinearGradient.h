/*
 * LinearGradient.h
 *
 *  Created on: Apr 5, 2018
 *      Author: florian
 */

#ifndef BASIC_LINEARGRADIENT_H_
#define BASIC_LINEARGRADIENT_H_

#include <CGAL/Vector_2.h>

#include "../GeomData.h"

namespace laby::aniso {
class Pin;
} /* namespace laby::aniso */


namespace laby::basic {

class LinearGradient {
public:
    LinearGradient(Point_2 startPoint, double thickness1, const Point_2& endPoint,
                   double thickness2);

    auto thickness(const Point_2& point) -> double;
    static auto f(double position, double startThickness, double endThickness) -> double;

private:
    double _thickness1;
    double _thickness2;
    double _sq_length;
    Point_2 _origin;
    CGAL::Vector_2<Kernel> _vec;
};

} // namespace laby::basic


#endif /* BASIC_LINEARGRADIENT_H_ */
