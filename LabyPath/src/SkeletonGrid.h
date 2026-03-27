/*
 * SkeletonGrid.h
 *
 *  Created on: Jul 9, 2018
 *      Author: florian
 */

#ifndef SKELETONGRID_H_
#define SKELETONGRID_H_

#include <CGAL/Bbox_2.h>
#include <CGAL/Kernel/interface_macros.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <vector>
#include "protoc/AllConfig.pb.h"
#include "GeomData.h"

#include "SVGParser/Loader.h"
namespace laby {

class SkeletonGrid {
public:
    const CGAL::Bbox_2& bbox() const { return _bbox; }

    const std::vector<Segment_info_2>& segResult() const { return _segResult; }

    SkeletonGrid(const proto::SkeletonGrid& config);
    void medialGraph(const std::vector<CGAL::Polygon_with_holes_2<Kernel>>& polygons, const double& distance);
    void create(const svgp::Loader& vRibbons);

private:
    const proto::SkeletonGrid _config;
    std::vector<Kernel::Segment_2> _radialList;
    std::vector<Kernel::Segment_2> _circularList;
    CGAL::Bbox_2 _bbox;
    std::vector<Segment_info_2> _segResult;
};

} /* namespace laby */

#endif /* SKELETONGRID_H_ */
