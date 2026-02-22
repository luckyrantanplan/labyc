/*
 * SVGShapeToGrid.h
 *
 *  Created on: Jun 21, 2018
 *      Author: florian
 */

#ifndef SVGSHAPETOGRID_H_
#define SVGSHAPETOGRID_H_

#include <bits/stdint-intn.h>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <CGAL/Arr_extended_dcel.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arr_simple_point_location.h>
#include <CGAL/Arrangement_2/Arrangement_2_iterators.h>
#include <CGAL/Arrangement_with_history_2.h>
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

    typedef CGAL::Arr_segment_traits_2<Kernel> Traits_2;

    typedef CGAL::Arr_face_extended_dcel<Traits_2, FaceNonZeroData> Dcel;

    typedef Traits_2::Point_2 Point_2;
    typedef Traits_2::Curve_2 Segment_2;
    typedef CGAL::Arrangement_with_history_2<Traits_2, Dcel> Arr_with_hist_2;
    typedef Arr_with_hist_2::Curve_handle Curve_handle;
    typedef Arr_with_hist_2::Curve_handle::value_type Curve;
    typedef Arr_with_hist_2::Face_handle::value_type Face;
    typedef Arr_with_hist_2::Halfedge_handle::value_type Halfedge;
    typedef CGAL::Arr_simple_point_location<Arr_with_hist_2> Point_location;
    typedef CGAL::Straight_skeleton_2<Kernel> Ss;
    typedef boost::shared_ptr<Ss> SsPtr;

    static std::vector<Segment_2> get_grid(const Ribbon& ribb);

    static std::vector<Segment_2> get_grid(Arr_with_hist_2& arr);

    static std::vector<Segment_2> addToSegments(const Ribbon& ribb);

    static std::vector<CGAL::Polygon_with_holes_2<Kernel> > get_polygons(const Ribbon& ribb);

private:
    static void markFace(Halfedge& twin, Face& face, std::queue<Face*>& q);
    static void markWindingRule(Arr_with_hist_2& arr);
    static std::vector<CGAL::Polygon_with_holes_2<Kernel> > get_polygons(Arr_with_hist_2& arr);
    static std::vector<Segment_2> createOffset(std::vector<CGAL::Polygon_with_holes_2<Kernel>>& res, const double l);
    static void debugPoly(const CGAL::Polygon_2<Kernel>& outP);
};

} /* namespace laby */

#endif /* SVGSHAPETOGRID_H_ */
