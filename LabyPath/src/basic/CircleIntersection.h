/*
 * CircleIntersection.h
 *
 *  Created on: Jul 5, 2018
 *      Author: florian
 */

#ifndef BASIC_CIRCLEINTERSECTION_H_
#define BASIC_CIRCLEINTERSECTION_H_

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>

namespace laby {
namespace basic {

class CircleIntersection {
public:
    typedef CGAL::Exact_predicates_exact_constructions_kernel Kernel;

    static std::vector<Kernel::Point_2> prob_2(const Kernel::Point_2& c, double offset, const Kernel::Point_2& a, const Kernel::Point_2& b);

};

} /* namespace basic */
} /* namespace laby */

#endif /* BASIC_CIRCLEINTERSECTION_H_ */
