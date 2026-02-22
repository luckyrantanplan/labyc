/*
 * VoronoiSeg.h
 *
 *  Created on: Oct 25, 2017
 *      Author: florian
 */

#ifndef VORONOISEG_H_
#define VORONOISEG_H_

#include <CGAL/Exact_predicates_exact_constructions_kernel_with_sqrt.h>
#include <CGAL/Kernel/interface_macros.h>
#include <CGAL/Object.h>
#include <CGAL/Parabola_segment_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Line_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/Segment_Delaunay_graph_2/Are_same_points_C2.h>
#include <CGAL/Segment_Delaunay_graph_2/Traits_base_2.h>
#include <CGAL/Segment_Delaunay_graph_2.h>
#include <CGAL/Segment_Delaunay_graph_site_2.h>
#include <CGAL/Segment_Delaunay_graph_storage_traits_2.h>
#include <CGAL/Segment_Delaunay_graph_traits_2.h>
#include <cstddef>
#include <list>
#include <utility>
#include <vector>
#include <CGAL/Point_set_2.h>
#include "basic/AugmentedPolygonSet.h"
#include "GeomData.h"
#include "Ribbon.h"

namespace laby {
typedef CGAL::Exact_predicates_exact_constructions_kernel_with_sqrt Kernel_sqrt;

struct Cropped_voronoi_from_delaunay {

    typedef Kernel_sqrt::Segment_2 Segment_2;
    typedef Kernel_sqrt::Ray_2 Ray_2;
    typedef Kernel_sqrt::Line_2 Line_2;
    typedef Kernel_sqrt::Iso_rectangle_2 Iso_rectangle_2;
    typedef CGAL::Segment_Delaunay_graph_traits_2<Kernel_sqrt> Gt;
    typedef CGAL::Segment_Delaunay_graph_storage_traits_2<Gt> St;
    typedef CGAL::Segment_Delaunay_graph_2<Gt> SDG2;

    struct CustomParabola: public CGAL::Parabola_segment_2<Gt> {

        typedef Kernel_sqrt::Point_2 Point_2;
        typedef Kernel_sqrt::Segment_2 Segment_2;
        typedef Kernel_sqrt::Ray_2 Ray_2;
        typedef Kernel_sqrt::Line_2 Line_2;
        typedef Kernel_sqrt::Iso_rectangle_2 Iso_rectangle_2;

        explicit CustomParabola(const CGAL::Parabola_segment_2<Gt>& ps) :
                CGAL::Parabola_segment_2<Gt>(ps) {

        }

        Point_2 computeBezier();

        Point_2 getP1() const {
            return p1;
        }

        Point_2 getP2() const {
            return p2;
        }

        Kernel_sqrt::Line_2 get_tangent(const Point_2& p);
    };

    std::vector<Segment_2> m_cropped_vd;
    Iso_rectangle_2 m_bbox;

    explicit Cropped_voronoi_from_delaunay(Iso_rectangle_2 i_bbox) :
            m_cropped_vd { }, m_bbox { i_bbox } {

    }

    Cropped_voronoi_from_delaunay() :
            m_cropped_vd { }, m_bbox { } {

    }

    template<class RSL>
    void crop_and_extract_segment(const RSL& rsl) {
        if (!rsl.is_degenerate()) {
            auto obj = CGAL::intersection(rsl, m_bbox);
            if (obj) {
                const Segment_2* s = boost::get<Segment_2>(&*obj);

                if (s)
                    m_cropped_vd.push_back(*s);
            }
        }
    }

    void draw_dual(const SDG2& sdg);

    bool same_points(const SDG2& sdg, const SDG2::Site_2& p, const SDG2::Site_2& q) const {
        return sdg.geom_traits().equal_2_object()(p, q);
    }

    bool is_endpoint_of_segment(const SDG2& sdg, const SDG2::Site_2& p, const SDG2::Site_2& s) const {

        return (same_points(sdg, p, s.source_site()) || same_points(sdg, p, s.target_site()));
    }

};

class VoronoiMedialSkeleton {
public:
    typedef CGAL::Segment_Delaunay_graph_traits_2<Kernel_sqrt> Gt;
    typedef CGAL::Segment_Delaunay_graph_storage_traits_2<Gt> St;
    typedef CGAL::Segment_Delaunay_graph_2<Gt> SDG2;
    typedef std::vector<Kernel::Point_2> IntersectType;

    explicit VoronoiMedialSkeleton(const CGAL::Polygon_with_holes_2<Kernel>& polygon);

    VoronoiMedialSkeleton(const Ribbon& ribbon, const CGAL::Bbox_2& bbox);
    VoronoiMedialSkeleton(const std::vector<Segment_info_2>& seg_list, const CGAL::Bbox_2& bbox);

    basic::Arrangement_2Node cutAndGetArrangementSkeleton(const CGAL::Polygon_with_holes_2<Kernel>& polygon) const;

    Arrangement_2 getSimpleArr(const Ribbon& rib, const Ribbon& ribLimit) const;

    const std::vector<Kernel::Segment_2>& get_vor_segments() const {
        return _result;
    }

private:
    static void addPolygon(const CGAL::Polygon_with_holes_2<Kernel>& polygon, SDG2& sdg);
    static void addSimplePolygon(const CGAL::Polygon_2<Kernel>& outerBoundary, std::vector<Gt::Point_2>& points, std::vector<std::pair<std::size_t, std::size_t> >& indices);
    static void addPolyline(const Polyline& pl, std::vector<Gt::Point_2>& points, std::vector<std::pair<std::size_t, std::size_t> >& indices);

    static std::vector<basic::SegmentNode> snap_rounding(const std::vector<basic::SegmentNode>& result2, const basic::Arrangement_2Node& polygon_arr);

    //hidden parameter in getCachePoint : double epsilon = 0.00001;

    static const Point_2 getCachePoint(const Point_2& pt, CGAL::Point_set_2<Kernel>& pt_set);
    static void setCachePoint(const Point_2& pt, CGAL::Point_set_2<Kernel>& pt_set, std::map<Point_2, Point_2>& map_cache);

    static const Point_2& getConstCachePoint(const Point_2& pt, CGAL::Point_set_2<Kernel>& pt_set);
    static bool snapPoint(const Point_2& pt, CGAL::Point_set_2<Kernel>& pt_set);
    Arrangement_2 snapRoundingArr(CGAL::Point_set_2<Kernel> pt_set, const Arrangement_2& arr) const;
    static Point_2 extends_to_projection(const Kernel::Point_2& a, const Halfedge& he2);
    static Arrangement_2 extendsAntennaArr(const Arrangement_2& arr);
    static Arrangement_2 removeAntennaArr(const Arrangement_2& arr);

    std::vector<Kernel::Segment_2> _result;

}
;

} /* namespace laby */
#endif /* VORONOISEG_H_ */
