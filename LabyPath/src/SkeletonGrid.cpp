/*
 * SkeletonGrid.cpp
 *
 *  Created on: Jul 9, 2018
 *      Author: florian
 */

#include "SkeletonGrid.h"

#include <CGAL/Arr_geometry_traits/Curve_data_aux.h>
#include <CGAL/Arrangement_2/Arrangement_on_surface_2_global.h>
#include <svgpp/factory/integer_color.hpp>
#include <iostream>

#include "basic/AugmentedPolygonSet.h"
#include "basic/Color.h"
#include "Rendering/GraphicRendering.h"
#include "Ribbon.h"
#include "SkeletonOffset.h"
#include "SkeletonRadial.h"
#include "SVGShapeToGrid.h"
#include "VoronoiMedialSkeleton.h"

namespace laby {

SkeletonGrid::SkeletonGrid(const proto::SkeletonGrid& config) : _config(config) {

    svgp::Loader load(config.inputfile());
    for (Ribbon& rib : load.ribList()) {
        rib.simplify(config.simplificationoforiginalsvg());
    }

    create(load);
}
void SkeletonGrid::create(const svgp::Loader& load) {
    std::vector<Ribbon> result;
    result.reserve(load.ribList().size());

    int32_t ribNumber = 0;

    for (const Ribbon& rib : load.ribList()) {
        std::vector<CGAL::Polygon_with_holes_2<Kernel>> polygons = SVGShapeToGrid::get_polygons(rib);

        const double blue = laby::basic::Color::get_blue_normalized(static_cast<uint32_t>(rib.fill_color()));

        const uint32_t fillColor = laby::basic::Color::set_blue(static_cast<uint32_t>(rib.fill_color()), static_cast<uint32_t>(ribNumber));
        ++ribNumber;

        const double distance = (_config.max_sep() - _config.min_sep()) * blue + _config.min_sep();

        medialGraph(polygons, distance);

        {
            std::vector<Segment_info_2> segResult;
            for (const Kernel::Segment_2& seg : _circularList) {
                segResult.push_back(Segment_info_2(seg, EdgeInfo{static_cast<int32_t>(fillColor), EdgeInfo::Coordinate{0}}));
            }
            Arrangement_2 arr;
            CGAL::insert(arr, segResult.begin(), segResult.end());

            result.emplace_back(Ribbon::createRibbonOfEdge(arr, 0.1));

            result.back().set_fill_color(static_cast<int32_t>(laby::basic::Color::set_green(fillColor, 50)));
        }
        {
            std::vector<Segment_info_2> segResult;
            for (const Kernel::Segment_2& seg : _radialList) {
                segResult.push_back(Segment_info_2(seg, EdgeInfo{static_cast<int32_t>(fillColor + 1), EdgeInfo::Coordinate{0}}));
            }
            Arrangement_2 arr;
            CGAL::insert(arr, segResult.begin(), segResult.end());

            result.emplace_back(Ribbon::createRibbonOfEdge(arr, 0.1));
            result.back().set_fill_color(static_cast<int32_t>(laby::basic::Color::set_green(fillColor, 100)));
        }
        result.emplace_back(rib);
        result.back().set_fill_color(static_cast<int32_t>(laby::basic::Color::set_green(fillColor, 150)));
    }

    std::cout << "GraphicRendering::printRibbonSvg(load.viewBox()" << load.viewBox() << std::endl;
    GraphicRendering::printRibbonSvg(load.viewBox(), _config.outputfile(), _config.min_sep() / 3., result);
}

void SkeletonGrid::medialGraph(const std::vector<CGAL::Polygon_with_holes_2<Kernel>>& polygons, const double& distance) {
    _circularList.clear();
    _radialList.clear();
    std::vector<Kernel::Segment_2> result2;

    for (const CGAL::Polygon_with_holes_2<Kernel>& polygon : polygons) {
        VoronoiMedialSkeleton vor = VoronoiMedialSkeleton(polygon);
        std::cout << " cut ok " << std::endl;

        laby::basic::Arrangement_2Node arr3 = vor.cutAndGetArrangementSkeleton(polygon);

        SkeletonRadial::Config configRadial;
        configRadial.sep = distance;
        configRadial.filtered_distance = distance / 5.; //??
        configRadial.min_length_polyline = configRadial.sep / 2.;
        configRadial.seed = _config.seed();
        SkeletonRadial radial(configRadial);

        radial.create_radial(arr3, polygon);

        SkeletonOffset::create_all_offsets(distance, arr3, _circularList);
        const auto listSeg = radial.radialList();
        _radialList.insert(_radialList.end(), listSeg.begin(), listSeg.end());
    }
    std::cout << " result2 " << std::endl;
}

} /* namespace laby */
