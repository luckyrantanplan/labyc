/*
 * SkeletonOffset.h
 *
 *  Created on: Jul 9, 2018
 *      Author: florian
 */

#ifndef SKELETONOFFSET_H_
#define SKELETONOFFSET_H_

#include "basic/AugmentedPolygonSet.h"
#include "GeomData.h"
#include "SkeletonGrid.h"

namespace laby {

class SkeletonOffset {

public:
    using IntersectType = std::vector<Kernel::Point_2>;
    static void createAllOffsets(const double& distance, const basic::Arrangement_2Node& arr3, std::vector<Kernel::Segment_2>& result2);

    static void createOffset(const basic::Arrangement_2Node& arr3, double offset_distance, std::vector<Kernel::Segment_2>& result2);

    static auto getPolygonOffset(const std::vector<Kernel::Segment_2>& result2) -> std::vector<CGAL::Polygon_with_holes_2<Kernel>>;
private:
    static auto getSegment(const basic::HalfedgeNode& he2) -> Kernel::Segment_2;
    static void offsetFace(const basic::HalfedgeNode& he, std::vector<Kernel::Segment_2>& result2, const double& offset_distance,
            std::unordered_map<const basic::SegmentNode*, IntersectType>& vertices_cache);
    static void offsetCorner(const basic::HalfedgeNode& he, std::vector<Kernel::Segment_2>& result2, const double& offset_distance,
            std::unordered_map<const basic::SegmentNode*, IntersectType>& vertices_cache);
    static void addToSegmentsList(const IntersectType& result, bool& winding, std::vector<Kernel::Segment_2>& result2, Kernel::Point_2& last_point);
    static auto getLineSegmentIntersect(const Kernel::Line_2& line, const Kernel::Segment_2& seg) -> IntersectType;

};

} /* namespace laby */

#endif /* SKELETONOFFSET_H_ */
