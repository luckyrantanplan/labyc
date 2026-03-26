/*
 * AugmentedPolygonSet.h
 *
 *  Created on: Apr 19, 2018
 *      Author: florian
 */

#ifndef BASIC_AUGMENTEDPOLYGONSET_H_
#define BASIC_AUGMENTEDPOLYGONSET_H_

#include <cstdint>
#include <CGAL/Arr_consolidated_curve_data_traits_2.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arrangement_2.h>
#include <CGAL/Boolean_set_operations_2/Gps_default_dcel.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/General_polygon_set_2.h>
#include <CGAL/Gps_segment_traits_2.h>
#include <CGAL/Point_2.h>
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

    explicit EdgeNodeInfo(int32_t polygonId) :
            _polygonId { polygonId } {

    }

    bool operator ==(const EdgeNodeInfo& it) const {
        return _polygonId == it._polygonId;
    }

    void print(std::ostream& os) const {
        os << _polygonId;
    }
};

class FaceNodeInfo: public CGAL::Gps_face_base {
public:
    FaceNodeInfo() {

    }
    virtual ~FaceNodeInfo() {

    }
    void setPolygonId(int32_t polygonId) {
        if (contained()) {
            _polygonIds.emplace(polygonId);
        }
    }
    std::unordered_set<int32_t> _polygonIds;

    void set_data(const std::unordered_set<int32_t>& data) {
        _polygonIds = data;
    }

    std::unordered_set<int32_t>& data() {
        return _polygonIds;
    }

    const std::unordered_set<int32_t>& data() const {
        return _polygonIds;
    }
    /*! Assign from another face. */
    virtual void assign(const Arr_face_base& f) {
        Gps_face_base::assign(f);

        const FaceNodeInfo& ex_f = static_cast<const FaceNodeInfo&>(f);
        _polygonIds = ex_f._polygonIds;
    }

};

struct Overlay_label {
    std::unordered_set<int32_t> operator()(const std::unordered_set<int32_t>& a, const std::unordered_set<int32_t>& b) const {
        std::unordered_set<int32_t> fn;
        fn.insert(a.begin(), a.end());
        fn.insert(b.begin(), b.end());
        return fn;
    }
};

typedef CGAL::Exact_predicates_exact_constructions_kernel KernelAug;
typedef std::vector<CGAL::Point_2<KernelAug>> ContainerNode;
typedef CGAL::Arr_segment_traits_2<KernelAug> Segment_traits_2;
typedef CGAL::Arr_consolidated_curve_data_traits_2<Segment_traits_2, EdgeNodeInfo> SegTraitNode;

// Use plain segment traits for Gps_segment_traits_2 to avoid
// Polygon_2_curve_iterator incompatibility with curve data traits in CGAL 5.x.
typedef CGAL::Gps_segment_traits_2<KernelAug, ContainerNode> GpsSegTraitNode;

typedef CGAL::Arr_dcel_base<CGAL::Arr_vertex_base<typename GpsSegTraitNode::Point_2>, CGAL::Gps_halfedge_base<typename GpsSegTraitNode::X_monotone_curve_2>, FaceNodeInfo> DcelNode;

typedef CGAL::General_polygon_set_2<GpsSegTraitNode, DcelNode> Polygon_set_2Node;

typedef Polygon_set_2Node::Arrangement_2 Arrangement_2Node;
typedef Polygon_set_2Node::Polygon_2 Polygon_2Node;
typedef Polygon_set_2Node::Polygon_with_holes_2 Polygon_with_holes_2Node;
typedef Arrangement_2Node::Halfedge HalfedgeNode;
typedef HalfedgeNode::X_monotone_curve SegmentNode;
typedef Arrangement_2Node::Face FaceNode;
typedef CGAL::Arr_face_overlay_traits<Arrangement_2Node, //
        Arrangement_2Node, //
        Arrangement_2Node, //
        Overlay_label> Overlay_traitsNode;

} /* namespace basic */

// Helper to check if a halfedge is on the boundary of a polygon with the given ID.
// Replaces the old pattern: he.curve().data().find(EdgeNodeInfo(id)) != he.curve().data().end()
// which relied on Arr_consolidated_curve_data_traits_2 (incompatible with CGAL 5.x).
namespace basic {
inline bool edgeHasPolygonId(const HalfedgeNode& he, int32_t id) {
    bool faceHas = he.face()->data().count(id) > 0;
    bool twinFaceHas = he.twin()->face()->data().count(id) > 0;
    return faceHas || twinFaceHas;
}
}

} /* namespace laby */

#endif /* BASIC_AUGMENTEDPOLYGONSET_H_ */
