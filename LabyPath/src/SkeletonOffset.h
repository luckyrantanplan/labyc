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
    typedef std::vector<Kernel::Point_2> IntersectType;
    static void create_all_offsets(const double& distance, const basic::Arrangement_2Node& arr3, std::vector<Kernel::Segment_2>& result2);

    static void create_offset(const basic::Arrangement_2Node& arr3, double offset_distance, std::vector<Kernel::Segment_2>& result2);

    static std::vector<CGAL::Polygon_with_holes_2<Kernel>> get_polygon_offset(const std::vector<Kernel::Segment_2>& result2);
private:
    static Kernel::Segment_2 getSegment(const basic::HalfedgeNode& he2);
    static void offset_face(const basic::HalfedgeNode& he, std::vector<Kernel::Segment_2>& result2, const double& offset_distance,
            std::unordered_map<const basic::SegmentNode*, IntersectType>& vertices_cache);
    static void offset_corner(const basic::HalfedgeNode& he, std::vector<Kernel::Segment_2>& result2, const double& offset_distance,
            std::unordered_map<const basic::SegmentNode*, IntersectType>& vertices_cache);
    static void addToSegmentsList(const IntersectType& result, bool& winding, std::vector<Kernel::Segment_2>& result2, Kernel::Point_2& last_point);
    static IntersectType getLineSegmentIntersect(const Kernel::Line_2& line, const Kernel::Segment_2& seg);

};

} /* namespace laby */

#endif /* SKELETONOFFSET_H_ */
