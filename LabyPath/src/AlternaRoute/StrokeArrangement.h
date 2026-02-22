/*
 * StrokeArrangement.h
 *
 *  Created on: Aug 27, 2018
 *      Author: florian
 */

#ifndef ALTERNAROUTE_STROKEARRANGEMENT_H_
#define ALTERNAROUTE_STROKEARRANGEMENT_H_

#include <bits/stdint-intn.h>
#include <CGAL/Arr_curve_data_traits_2.h>
#include <CGAL/Arr_extended_dcel.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arrangement_2.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <complex>
#include <iostream>
#include <optional>
#include <unordered_map>
#include <utility>
#include <CGAL/Polygon_2.h>

namespace laby {
namespace alter {

class TrapezeEdgeInfo;

class Dummy {

};
typedef CGAL::Exact_predicates_exact_constructions_kernel Kernel;

typedef CGAL::Arr_segment_traits_2<Kernel> Segment_traits_2;

typedef CGAL::Arr_curve_data_traits_2<Segment_traits_2, TrapezeEdgeInfo> TrapezeTraits_2;

typedef TrapezeTraits_2::X_monotone_curve_2 Segment_trapeze_info_2;

typedef CGAL::Arr_extended_dcel<TrapezeTraits_2, Dummy, Dummy, Dummy> TrapezeDcel;

typedef CGAL::Arrangement_2<TrapezeTraits_2, TrapezeDcel> ArrTrapeze;

struct Offset_triplet {

    Kernel::Point_2 origin;
    Kernel::Point_2 offset1;
    Kernel::Point_2 offset2;

    void print(std::ostream& os) const {
        os << " offset1 " << offset1;
        os << " origin " << origin;
        os << "offset2 " << offset2;

    }
};

class TrapezeEdgeInfo {
public:

    TrapezeEdgeInfo();

    TrapezeEdgeInfo(const Offset_triplet& source, const Offset_triplet& target, const int32_t& direction);

    const CGAL::Polygon_2<Kernel> getGeometry(const Kernel::Segment_2& seg) const;

    int32_t direction() const {
        return _direction;
    }

private:
    static const Kernel::Point_2 intersection(const Kernel::Line_2& l, const Kernel::Line_2& h);
    void computeGeometry(const Kernel::Point_2& source, const Kernel::Point_2& target, CGAL::Polygon_2<Kernel>& geometry) const;

    int32_t _direction = 0;
    Offset_triplet _source;
    Offset_triplet _target;

};

} /* namespace alter */
} /* namespace laby */

#endif /* ALTERNAROUTE_STROKEARRANGEMENT_H_ */
