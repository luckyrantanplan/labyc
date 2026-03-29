/*
 * AugmentedPolygonSet.h
 *
 *  Created on: Apr 19, 2018
 *      Author: florian
 */

#ifndef BASIC_AUGMENTEDPOLYGONSET_H_
#define BASIC_AUGMENTEDPOLYGONSET_H_

#include <CGAL/Arr_consolidated_curve_data_traits_2.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arrangement_2.h>
#include <CGAL/Boolean_set_operations_2/Gps_default_dcel.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/General_polygon_set_2.h>
#include <CGAL/Gps_segment_traits_2.h>
#include <CGAL/Point_2.h>
#include <cstdint>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace laby {
namespace basic {

class EdgeNodeInfo {
  public:
    int32_t _polygonId = -1;

    explicit EdgeNodeInfo(int32_t polygonId) : _polygonId{polygonId} {}

    auto operator==(const EdgeNodeInfo& it) const -> bool {
        return _polygonId == it._polygonId;
    }

    void print(std::ostream& os) const {
        os << _polygonId;
    }
};

class FaceNodeInfo : public CGAL::Gps_face_base {
  public:
    FaceNodeInfo() = default;
    FaceNodeInfo(const FaceNodeInfo&) = default;
    FaceNodeInfo(FaceNodeInfo&&) noexcept = default;
    auto operator=(const FaceNodeInfo&) -> FaceNodeInfo& = default;
    auto operator=(FaceNodeInfo&&) noexcept -> FaceNodeInfo& = default;
    ~FaceNodeInfo() override = default;

    void setPolygonId(int32_t polygonId) {
        if (contained()) {
            _polygonIds.emplace(polygonId);
        }
    }
    std::unordered_set<int32_t> _polygonIds{};

    void setData(const std::unordered_set<int32_t>& data) {
        _polygonIds = data;
    }

    void setData(const std::unordered_set<int32_t>& data) {
        setData(data);
    }

    auto data() -> std::unordered_set<int32_t>& {
        return _polygonIds;
    }

    auto data() const -> const std::unordered_set<int32_t>& {
        return _polygonIds;
    }
    /*! Assign from another face. */
    void assign(const Arr_face_base& f) override {
        Gps_face_base::assign(f);

        // CGAL passes back the exact derived face type here.
        const auto& exFace = dynamic_cast<const FaceNodeInfo&>(f);
        _polygonIds = exFace._polygonIds;
    }
};

struct OverlayLabel {
    auto operator()(const std::unordered_set<int32_t>& a,
                    const std::unordered_set<int32_t>& b) const -> std::unordered_set<int32_t> {
        std::unordered_set<int32_t> fn;
        fn.insert(a.begin(), a.end());
        fn.insert(b.begin(), b.end());
        return fn;
    }
};

using KernelAug = CGAL::Exact_predicates_exact_constructions_kernel;
using ContainerNode = std::vector<CGAL::Point_2<KernelAug>>;
using Segment_traits_2 = CGAL::Arr_segment_traits_2<KernelAug>;
using SegTraitNode = CGAL::Arr_consolidated_curve_data_traits_2<Segment_traits_2, EdgeNodeInfo>;

// Use plain segment traits for Gps_segment_traits_2 to avoid
// Polygon_2_curve_iterator incompatibility with curve data traits in CGAL 5.x.
using GpsSegTraitNode = CGAL::Gps_segment_traits_2<KernelAug, ContainerNode>;

using DcelNode =
    CGAL::Arr_dcel_base<CGAL::Arr_vertex_base<typename GpsSegTraitNode::Point_2>,
                        CGAL::Gps_halfedge_base<typename GpsSegTraitNode::X_monotone_curve_2>,
                        FaceNodeInfo>;

using Polygon_set_2Node = CGAL::General_polygon_set_2<GpsSegTraitNode, DcelNode>;

using Arrangement_2Node = Polygon_set_2Node::Arrangement_2;
using Polygon_2Node = Polygon_set_2Node::Polygon_2;
using Polygon_with_holes_2Node = Polygon_set_2Node::Polygon_with_holes_2;
using HalfedgeNode = Arrangement_2Node::Halfedge;
using SegmentNode = HalfedgeNode::X_monotone_curve;
using FaceNode = Arrangement_2Node::Face;
using Overlay_traitsNode = CGAL::Arr_face_overlay_traits<Arrangement_2Node, //
                                                         Arrangement_2Node, //
                                                         Arrangement_2Node, //
                                                         OverlayLabel>;

} /* namespace basic */

// Helper to check if a halfedge is on the boundary of a polygon with the given ID.
// Replaces the old pattern: he.curve().data().find(EdgeNodeInfo(id)) != he.curve().data().end()
// which relied on Arr_consolidated_curve_data_traits_2 (incompatible with CGAL 5.x).
namespace basic {
inline auto edgeHasPolygonId(const HalfedgeNode& he, int32_t id) -> bool {
    const bool faceHas = he.face()->data().count(id) > 0;
    const bool twinFaceHas = he.twin()->face()->data().count(id) > 0;
    return faceHas != twinFaceHas;
}
} // namespace basic

} /* namespace laby */

#endif /* BASIC_AUGMENTEDPOLYGONSET_H_ */
