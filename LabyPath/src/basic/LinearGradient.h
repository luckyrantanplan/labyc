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

namespace laby {
namespace basic {

class LinearGradient {
public:
    LinearGradient(const Point_2& p1, const double thickness1, const Point_2& p2, const double thickness2);

    double thickness(const Point_2& p);
    double f(double t, double _thickness1, double _thickness2);

private:
    double _thickness1;
    double _thickness2;
    double _sq_length;
    Point_2 origin;
    CGAL::Vector_2<Kernel> _vec;
};

} /* namespace basic */
} /* namespace laby */

#endif /* BASIC_LINEARGRADIENT_H_ */
