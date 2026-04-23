/*
 * VoronoiSeg.h
 *
 *  Created on: Oct 25, 2017
 *      Author: florian
 */

#ifndef VORONOISEG_H_
#define VORONOISEG_H_

#include "GeomData.h"
#include "Ribbon.h"
#include "basic/AugmentedPolygonSet.h"
#include <CGAL/Exact_predicates_exact_constructions_kernel_with_sqrt.h>
#include <CGAL/Kernel/interface_macros.h>
#include <CGAL/Line_2.h>
#include <CGAL/Object.h>
#include <CGAL/Parabola_segment_2.h>
#include <CGAL/Point_set_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/Segment_Delaunay_graph_2.h>
#include <CGAL/Segment_Delaunay_graph_2/Are_same_points_C2.h>
#include <CGAL/Segment_Delaunay_graph_2/Traits_base_2.h>
#include <CGAL/Segment_Delaunay_graph_site_2.h>
#include <CGAL/Segment_Delaunay_graph_storage_traits_2.h>
#include <CGAL/Segment_Delaunay_graph_traits_2.h>
#include <cstddef>
#include <list>
#include <utility>
#include <variant>
#include <vector>

namespace laby {
using Kernel_sqrt = CGAL::Exact_predicates_exact_constructions_kernel_with_sqrt;

struct CroppedVoronoiFromDelaunay {

    using Segment_2 = Kernel_sqrt::Segment_2;
    using Ray_2 = Kernel_sqrt::Ray_2;
    using Line_2 = Kernel_sqrt::Line_2;
    using Iso_rectangle_2 = Kernel_sqrt::Iso_rectangle_2;
    using Gt = CGAL::Segment_Delaunay_graph_traits_2<Kernel_sqrt>;
    using St = CGAL::Segment_Delaunay_graph_storage_traits_2<Gt>;
    using SDG2 = CGAL::Segment_Delaunay_graph_2<Gt>;

    struct CustomParabola : public CGAL::Parabola_segment_2<Gt> {

        using Point_2 = Kernel_sqrt::Point_2;
        using Segment_2 = Kernel_sqrt::Segment_2;
        using Ray_2 = Kernel_sqrt::Ray_2;
        using Line_2 = Kernel_sqrt::Line_2;
        using Iso_rectangle_2 = Kernel_sqrt::Iso_rectangle_2;

        explicit CustomParabola(const CGAL::Parabola_segment_2<Gt>& parabolaSegment)
            : CGAL::Parabola_segment_2<Gt>(parabolaSegment) {}

        auto computeBezier() -> Point_2;

        [[nodiscard]] auto getP1() const -> Point_2 {
            return p1;
        }

        [[nodiscard]] auto getP2() const -> Point_2 {
            return p2;
        }

        auto getTangent(const Point_2& point) -> Kernel_sqrt::Line_2;
    };

    explicit CroppedVoronoiFromDelaunay(Iso_rectangle_2 i_bbox)
        : m_bbox{std::move(std::move(i_bbox))} {}

    CroppedVoronoiFromDelaunay() = default;

    template <class RSL> void cropAndExtractSegment(const RSL& rsl) {
        if (!rsl.is_degenerate()) {
            auto obj = CGAL::intersection(rsl, m_bbox);
            if (obj) {
                const Segment_2* segmentPointer = std::get_if<Segment_2>(&*obj);

                if (segmentPointer) {
                    m_cropped_vd.push_back(*segmentPointer);
                }
            }
        }
    }

    void drawDual(const SDG2& sdg);

    [[nodiscard]] auto getCroppedVd() const -> const std::vector<Segment_2>& {
        return m_cropped_vd;
    }

    [[nodiscard]] static auto samePoints(const SDG2& sdg, const SDG2::Site_2& firstSite,
                                         const SDG2::Site_2& secondSite) -> bool {
        return sdg.geom_traits().equal_2_object()(firstSite, secondSite);
    }

    [[nodiscard]] static auto isEndpointOfSegment(const SDG2& sdg, const SDG2::Site_2& site,
                                                  const SDG2::Site_2& segmentSite) -> bool {

        return (samePoints(sdg, site, segmentSite.source_site()) ||
                samePoints(sdg, site, segmentSite.target_site()));
    }

  private:
    std::vector<Segment_2> m_cropped_vd;
    Iso_rectangle_2 m_bbox;
};

class VoronoiMedialSkeleton {
  public:
    using Gt = CGAL::Segment_Delaunay_graph_traits_2<Kernel_sqrt>;
    using St = CGAL::Segment_Delaunay_graph_storage_traits_2<Gt>;
    using SDG2 = CGAL::Segment_Delaunay_graph_2<Gt>;
    using IntersectType = std::vector<Kernel::Point_2>;

    explicit VoronoiMedialSkeleton(const CGAL::Polygon_with_holes_2<Kernel>& polygon);

    VoronoiMedialSkeleton(const Ribbon& ribbon, const CGAL::Bbox_2& bbox);
    VoronoiMedialSkeleton(const std::vector<Segment_info_2>& seg_list, const CGAL::Bbox_2& bbox);

    [[nodiscard]] auto cutAndGetArrangementSkeleton(
        const CGAL::Polygon_with_holes_2<Kernel>& polygon) const -> basic::Arrangement_2Node;

    [[nodiscard]] auto getSimpleArr(const Ribbon& rib,
                                    const Ribbon& ribLimit) const -> Arrangement_2;

    [[nodiscard]] auto getVorSegments() const -> const std::vector<Kernel::Segment_2>& {
        return _result;
    }

  private:
    static void addPolygon(const CGAL::Polygon_with_holes_2<Kernel>& polygon, SDG2& sdg);
    static void addSimplePolygon(const CGAL::Polygon_2<Kernel>& outerBoundary,
                                 std::vector<Gt::Point_2>& points,
                                 std::vector<std::pair<std::size_t, std::size_t>>& indices);
    static void addPolyline(const Polyline& polyline, std::vector<Gt::Point_2>& points,
                            std::vector<std::pair<std::size_t, std::size_t>>& indices);

    static auto
    snapRounding(const std::vector<basic::SegmentNode>& result2,
                 const basic::Arrangement_2Node& polygon_arr) -> std::vector<basic::SegmentNode>;

    // hidden parameter in getCachePoint : double epsilon = 0.00001;

    static auto getCachePoint(const Point_2& point,
                              CGAL::Point_set_2<Kernel>& pointSet) -> const Point_2;
    static void setCachePoint(const Point_2& point, CGAL::Point_set_2<Kernel>& pointSet,
                              std::map<Point_2, Point_2>& pointCache);

    static auto getConstCachePoint(const Point_2& point,
                                   CGAL::Point_set_2<Kernel>& pointSet) -> const Point_2&;
    static auto snapPoint(const Point_2& point, CGAL::Point_set_2<Kernel>& pointSet) -> bool;
    [[nodiscard]] auto snapRoundingArr(CGAL::Point_set_2<Kernel> pointSet,
                                       const Arrangement_2& arr) const -> Arrangement_2;
    static auto extendsToProjection(const Kernel::Point_2& projectionPoint,
                                    const Halfedge& halfedge) -> Point_2;
    static auto extendsAntennaArr(const Arrangement_2& arr) -> Arrangement_2;
    static auto removeAntennaArr(const Arrangement_2& arr) -> Arrangement_2;

    std::vector<Kernel::Segment_2> _result;
};

} /* namespace laby */
#endif /* VORONOISEG_H_ */
