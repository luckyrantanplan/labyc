/*
 * CircleIntersection.h
 *
 *  Created on: Jul 5, 2018
 *      Author: florian
 */

#ifndef BASIC_CIRCLEINTERSECTION_H_
#define BASIC_CIRCLEINTERSECTION_H_

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>


namespace laby::basic {

class CircleIntersection {
public:
    using Kernel = CGAL::Exact_predicates_exact_constructions_kernel;

    static auto prob2(const Kernel::Point_2& c, double offset, const Kernel::Point_2& a, const Kernel::Point_2& b) -> std::vector<Kernel::Point_2>;

};

} // namespace laby::basic


#endif /* BASIC_CIRCLEINTERSECTION_H_ */
