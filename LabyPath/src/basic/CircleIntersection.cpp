/*
 * CircleIntersection.cpp
 *
 *  Created on: Jul 5, 2018
 *      Author: florian
 */

#include "CircleIntersection.h"
#include <CGAL/Circle_2.h>
#include <CGAL/Circular_kernel_2/Intersection_traits.h>
#include <CGAL/Circular_kernel_intersections.h>
#include <CGAL/Exact_circular_kernel_2.h>
#include <vector>
#include <CGAL/Point_2.h>
#include <CGAL/Line_arc_2.h>
#include <CGAL/number_utils.h>
#include <utility>
#include <iterator>
#include <boost/variant/get.hpp>
#include <CGAL/Kernel/global_functions_2.h>


namespace laby::basic {

auto CircleIntersection::prob2(const Kernel::Point_2& center, double offset,
                               const Kernel::Point_2& segmentStart,
                               const Kernel::Point_2& segmentEnd)
    -> std::vector<CircleIntersection::Kernel::Point_2> {

    typedef CGAL::Exact_circular_kernel_2 Circular_k;
    typedef CGAL::Point_2<Circular_k> Point_2;
    typedef CGAL::Circle_2<Circular_k> Circle_2;
    typedef CGAL::Line_arc_2<Circular_k> Line_arc_2;

    Point_2 const centerCircular(CGAL::to_double(center.x()), CGAL::to_double(center.y()));
    Point_2 const startCircular(CGAL::to_double(segmentStart.x()), CGAL::to_double(segmentStart.y()));
    Point_2 const endCircular(CGAL::to_double(segmentEnd.x()), CGAL::to_double(segmentEnd.y()));

    Circle_2 const circle(centerCircular, offset * offset);

    Line_arc_2 const lineArc(startCircular, endCircular);
    typedef CGAL::CK2_Intersection_traits<Circular_k, Circle_2, Line_arc_2>::type Intersection_result;
    typedef std::pair<Circular_k::Circular_arc_point_2, unsigned int> ResultType;
    std::vector<Intersection_result> res;

    CGAL::intersection(circle, lineArc, std::back_inserter(res));
    std::vector<Kernel::Point_2> collect;

    for (const Intersection_result& result : res) {
        const ResultType& pair = boost::get<ResultType>(result);
        collect.emplace_back(CGAL::to_double(pair.first.x()), CGAL::to_double(pair.first.y()));

    }

    if (collect.size() == 2) {
        if (CGAL::has_larger_distance_to_point(segmentStart, collect.at(0), collect.at(1))) {
            std::swap(collect.at(0), collect.at(1));
        }
    }

    return collect;
}

} // namespace laby::basic

