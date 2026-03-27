/*
 * StrokeArrangement.h
 *
 *  Created on: Aug 27, 2018
 *      Author: florian
 */

#ifndef ALTERNAROUTE_STROKEARRANGEMENT_H_
#define ALTERNAROUTE_STROKEARRANGEMENT_H_

#include <CGAL/Arr_curve_data_traits_2.h>
#include <CGAL/Arr_extended_dcel.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arrangement_2.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Polygon_2.h>
#include <complex>
#include <cstdint>
#include <iostream>
#include <optional>
#include <unordered_map>
#include <utility>

namespace laby::alter {

class TrapezeEdgeInfo;

class Dummy {};
using Kernel = CGAL::Exact_predicates_exact_constructions_kernel;

using SegmentTraits2 = CGAL::Arr_segment_traits_2<Kernel>;

using TrapezeTraits2 = CGAL::Arr_curve_data_traits_2<SegmentTraits2, TrapezeEdgeInfo>;

using SegmentTrapezeInfo2 = TrapezeTraits2::X_monotone_curve_2;

using TrapezeDcel = CGAL::Arr_extended_dcel<TrapezeTraits2, Dummy, Dummy, Dummy>;

using ArrTrapeze = CGAL::Arrangement_2<TrapezeTraits2, TrapezeDcel>;

struct OffsetTriplet {

    Kernel::Point_2 origin;
    Kernel::Point_2 offset1;
    Kernel::Point_2 offset2;

    void print(std::ostream& outputStream) const {
        outputStream << " offset1 " << offset1;
        outputStream << " origin " << origin;
        outputStream << "offset2 " << offset2;
    }
};

class TrapezeEdgeInfo {
  public:
    struct SourceTriplet {
        OffsetTriplet value;
    };

    struct TargetTriplet {
        OffsetTriplet value;
    };

    TrapezeEdgeInfo();

    TrapezeEdgeInfo(SourceTriplet source, TargetTriplet target, int32_t direction);

    [[nodiscard]] auto
    getGeometry(const Kernel::Segment_2& segment) const -> CGAL::Polygon_2<Kernel>;

    [[nodiscard]] auto direction() const -> int32_t {
        return _direction;
    }

  private:
    static auto intersection(const Kernel::Line_2& lineA,
                             const Kernel::Line_2& lineB) -> Kernel::Point_2;
    void computeGeometry(const Kernel::Point_2& firstBoundaryPoint,
                         const Kernel::Point_2& secondBoundaryPoint,
                         CGAL::Polygon_2<Kernel>& geometry) const;

    int32_t _direction = 0;
    OffsetTriplet _source;
    OffsetTriplet _target;
};

} /* namespace laby::alter */

#endif /* ALTERNAROUTE_STROKEARRANGEMENT_H_ */
