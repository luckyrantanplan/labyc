/*
 * SVGShapeToGrid.h
 *
 *  Created on: Jun 21, 2018
 *      Author: florian
 */

#ifndef SVGSHAPETOGRID_H_
#define SVGSHAPETOGRID_H_

#include <cstdint>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <CGAL/Arr_extended_dcel.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arr_simple_point_location.h>
#include <CGAL/Arrangement_2/Arrangement_2_iterators.h>
#include <CGAL/Arrangement_with_history_2.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/In_place_list.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/Straight_skeleton_2.h>
#include <queue>
#include <vector>

#include "GeomData.h"
#include "Ribbon.h"

namespace laby {

class SVGShapeToGrid {

public:

    struct FaceNonZeroData {

        int32_t _winding_number = 0;
        bool _walked = false;
    };

    using Traits_2 = CGAL::Arr_segment_traits_2<Kernel>;

    using Dcel = CGAL::Arr_face_extended_dcel<Traits_2, FaceNonZeroData>;

    using Point_2 = Traits_2::Point_2;
    using Segment_2 = Traits_2::Curve_2;
    using Arr_with_hist_2 = CGAL::Arrangement_with_history_2<Traits_2, Dcel>;
    using Curve_handle = Arr_with_hist_2::Curve_handle;
    using Curve = Arr_with_hist_2::Curve_handle::value_type;
    using Face = Arr_with_hist_2::Face_handle::value_type;
    using Halfedge = Arr_with_hist_2::Halfedge_handle::value_type;
    using Point_location = CGAL::Arr_simple_point_location<Arr_with_hist_2>;
    using InexactK = CGAL::Exact_predicates_inexact_constructions_kernel;
    using Ss = CGAL::Straight_skeleton_2<InexactK>;
    using SsPtr = boost::shared_ptr<Ss>;

    static auto getGrid(const Ribbon& ribb) -> std::vector<Segment_2>;

    static auto getGrid(Arr_with_hist_2& arr) -> std::vector<Segment_2>;

    static auto addToSegments(const Ribbon& ribb) -> std::vector<Segment_2>;

    static auto getPolygons(const Ribbon& ribb) -> std::vector<CGAL::Polygon_with_holes_2<Kernel> >;

private:
    static void markFace(Halfedge& twin, Face& face, std::queue<Face*>& q);
    static void markWindingRule(Arr_with_hist_2& arr);
    static auto getPolygons(Arr_with_hist_2& arr) -> std::vector<CGAL::Polygon_with_holes_2<Kernel> >;
    static auto createOffset(std::vector<CGAL::Polygon_with_holes_2<Kernel>>& res, double l) -> std::vector<Segment_2>;
    static void debugPoly(const CGAL::Polygon_2<Kernel>& outP);
};

} /* namespace laby */

#endif /* SVGSHAPETOGRID_H_ */
