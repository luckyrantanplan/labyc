/*
 * SkeletonOffset.h
 *
 *  Created on: Jul 9, 2018
 *      Author: florian
 */

#ifndef SKELETONOFFSET_H_
#define SKELETONOFFSET_H_

#include "GeomData.h"
#include "SkeletonGrid.h"
#include "basic/AugmentedPolygonSet.h"

#include <unordered_map>
#include <vector>

namespace laby {

class SkeletonOffset {

  public:
    using IntersectType = std::vector<Kernel::Point_2>;

    static auto createAllOffsets(const double& distance,
                                 const basic::Arrangement_2Node& arrangement,
                                 std::vector<Kernel::Segment_2>& resultSegments) -> void;

    static auto createOffset(const basic::Arrangement_2Node& arrangement, double offsetDistance,
                             std::vector<Kernel::Segment_2>& resultSegments) -> void;

    static auto getPolygonOffset(const std::vector<Kernel::Segment_2>& resultSegments)
        -> std::vector<CGAL::Polygon_with_holes_2<Kernel>>;

  private:
    static auto getSegment(const basic::HalfedgeNode& halfedge) -> Kernel::Segment_2;

    static auto
    offsetFace(const basic::HalfedgeNode& halfedge, std::vector<Kernel::Segment_2>& resultSegments,
               const double& offsetDistance,
               std::unordered_map<const basic::SegmentNode*, IntersectType>& verticesCache) -> void;

    static auto offsetCorner(
        const basic::HalfedgeNode& halfedge, std::vector<Kernel::Segment_2>& resultSegments,
        const double& offsetDistance,
        std::unordered_map<const basic::SegmentNode*, IntersectType>& verticesCache) -> void;

    static auto addToSegmentsList(const IntersectType& intersectionPoints, bool& winding,
                                  std::vector<Kernel::Segment_2>& resultSegments,
                                  Kernel::Point_2& lastPoint) -> void;

    static auto getLineSegmentIntersect(const Kernel::Line_2& line,
                                        const Kernel::Segment_2& segment) -> IntersectType;
};

} /* namespace laby */

#endif /* SKELETONOFFSET_H_ */
