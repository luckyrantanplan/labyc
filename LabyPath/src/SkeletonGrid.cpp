/*
 * SkeletonGrid.cpp
 *
 *  Created on: Jul 9, 2018
 *      Author: florian
 */

#include "SkeletonGrid.h"

#include <CGAL/Arrangement_2/Arrangement_on_surface_2_global.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "GeomData.h"
#include "Rendering/GraphicRendering.h"
#include "Ribbon.h"
#include "SVGParser/Loader.h"
#include "SVGShapeToGrid.h"
#include "SkeletonOffset.h"
#include "SkeletonRadial.h"
#include "VoronoiMedialSkeleton.h"
#include "basic/AugmentedPolygonSet.h"
#include "basic/Color.h"
#include "protoc/AllConfig.pb.h"

namespace laby {

namespace {

constexpr double kRibbonRenderThickness = 0.1;
constexpr int32_t kCircularGreenValue = 50;
constexpr int32_t kRadialGreenValue = 100;
constexpr int32_t kOriginalGreenValue = 150;
constexpr double kOutputThicknessDivisor = 3.0;
constexpr double kFilteredDistanceDivisor = 5.0;
constexpr double kMinimumPolylineDivisor = 2.0;

} // namespace

SkeletonGrid::SkeletonGrid(proto::SkeletonGrid config) : _config(std::move(config)) {

    svgp::Loader load(_config.inputfile());
    for (Ribbon& rib : load.ribList()) {
        rib.simplify(_config.simplificationoforiginalsvg());
    }

    create(load);
}

auto SkeletonGrid::create(const svgp::Loader& load) -> void {
    std::vector<Ribbon> result;
    result.reserve(load.ribList().size());

    int32_t ribNumber = 0;

    for (const Ribbon& rib : load.ribList()) {
        std::vector<CGAL::Polygon_with_holes_2<Kernel>> const polygons =
            SVGShapeToGrid::getPolygons(rib);

        const double blue =
            laby::basic::Color::getBlueNormalized(static_cast<uint32_t>(rib.fillColor()));

        const uint32_t fillColor = laby::basic::Color::setBlue(
            static_cast<uint32_t>(rib.fillColor()), static_cast<uint32_t>(ribNumber));
        ++ribNumber;

        const double distance = (_config.max_sep() - _config.min_sep()) * blue + _config.min_sep();

        medialGraph(polygons, distance);

        {
            std::vector<Segment_info_2> segResult;
            segResult.reserve(_circularList.size());
            for (const Kernel::Segment_2& seg : _circularList) {
                segResult.emplace_back(
                    seg, EdgeInfo{static_cast<int32_t>(fillColor), EdgeInfo::Coordinate{0}});
            }
            Arrangement_2 arr;
            CGAL::insert(arr, segResult.begin(), segResult.end());

            result.emplace_back(Ribbon::createRibbonOfEdge(arr, kRibbonRenderThickness));

            result.back().setFillColor(
                static_cast<int32_t>(laby::basic::Color::setGreen(fillColor, kCircularGreenValue)));
        }
        {
            std::vector<Segment_info_2> segResult;
            segResult.reserve(_radialList.size());
            for (const Kernel::Segment_2& seg : _radialList) {
                segResult.emplace_back(
                    seg, EdgeInfo{static_cast<int32_t>(fillColor + 1), EdgeInfo::Coordinate{0}});
            }
            Arrangement_2 arr;
            CGAL::insert(arr, segResult.begin(), segResult.end());

            result.emplace_back(Ribbon::createRibbonOfEdge(arr, kRibbonRenderThickness));
            result.back().setFillColor(
                static_cast<int32_t>(laby::basic::Color::setGreen(fillColor, kRadialGreenValue)));
        }
        result.emplace_back(rib);
        result.back().setFillColor(
            static_cast<int32_t>(laby::basic::Color::setGreen(fillColor, kOriginalGreenValue)));
    }

    std::cout << "GraphicRendering::printRibbonSvg(load.viewBox()" << load.viewBox() << '\n';
    GraphicRendering::printRibbonSvg(load.viewBox(), _config.outputfile(),
                                     _config.min_sep() / kOutputThicknessDivisor, result);
}

auto SkeletonGrid::medialGraph(const std::vector<CGAL::Polygon_with_holes_2<Kernel>>& polygons,
                               const double& distance) -> void {
    _circularList.clear();
    _radialList.clear();
    std::vector<Kernel::Segment_2> const result2;

    for (const CGAL::Polygon_with_holes_2<Kernel>& polygon : polygons) {
        VoronoiMedialSkeleton const vor = VoronoiMedialSkeleton(polygon);
        std::cout << " cut ok " << '\n';

        laby::basic::Arrangement_2Node const arr3 = vor.cutAndGetArrangementSkeleton(polygon);

        SkeletonRadial::Config configRadial;
        configRadial.sep = distance;
        configRadial.filtered_distance = distance / kFilteredDistanceDivisor; //??
        configRadial.min_length_polyline = configRadial.sep / kMinimumPolylineDivisor;
        configRadial.seed = _config.seed();
        SkeletonRadial radial(configRadial);

        radial.createRadial(arr3, polygon);

        SkeletonOffset::createAllOffsets(distance, arr3, _circularList);
        const auto listSeg = radial.radialList();
        _radialList.insert(_radialList.end(), listSeg.begin(), listSeg.end());
    }
    std::cout << " result2 " << '\n';
}

} /* namespace laby */
