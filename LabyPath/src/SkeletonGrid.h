/*
 * SkeletonGrid.h
 *
 *  Created on: Jul 9, 2018
 *      Author: florian
 */

#ifndef SKELETONGRID_H_
#define SKELETONGRID_H_

#include "GeomData.h"
#include "protoc/AllConfig.pb.h"
#include <CGAL/Bbox_2.h>
#include <CGAL/Kernel/interface_macros.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <vector>

#include "SVGParser/Loader.h"
namespace laby {

class SkeletonGrid {
  public:
    [[nodiscard]] auto bbox() const -> const CGAL::Bbox_2& {
        return _bbox;
    }

    [[nodiscard]] auto segResult() const -> const std::vector<Segment_info_2>& {
        return _segResult;
    }

    explicit SkeletonGrid(proto::SkeletonGrid config);
    auto medialGraph(const std::vector<CGAL::Polygon_with_holes_2<Kernel>>& polygons,
                     const double& distance) -> void;
    auto create(const svgp::Loader& load) -> void;

  private:
    proto::SkeletonGrid _config;
    std::vector<Kernel::Segment_2> _radialList{};
    std::vector<Kernel::Segment_2> _circularList{};
    CGAL::Bbox_2 _bbox;
    std::vector<Segment_info_2> _segResult{};
};

} /* namespace laby */

#endif /* SKELETONGRID_H_ */
