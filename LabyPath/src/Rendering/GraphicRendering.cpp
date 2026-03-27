/*
 * GraphicRendering.cpp
 *
 *  Created on: Feb 21, 2018
 *      Author: florian
 */

#include "GraphicRendering.h"

#include <CGAL/Bbox_2.h>
#include <cstdint>
#include <iostream>
#include <string>
#include <svgpp/factory/integer_color.hpp>

#include "../GeomData.h"
#include "../OrientedRibbon.h"
#include "../SVGParser/Loader.h"
#include "../Smoothing.h"
#include "../basic/Color.h"

namespace laby {

namespace {
constexpr double kSvgCollapseEpsilon = 1e-12;
constexpr int32_t kDebugFaceRed = 250;
constexpr int32_t kDebugFaceGreen = 10;
constexpr int32_t kDebugFaceBlue = 10;
} // namespace

GraphicRendering::GraphicRendering(proto::GraphicRendering config) : _config(std::move(config)) {

    svgp::Loader load(_config.inputfile());

    _box = CGAL::Bbox_2(0, 0, load.viewBox().xmax(), load.viewBox().ymax());
    svg::Dimensions dimensions(_box.xmax(), _box.ymax());
    svg::DocumentSVG docSvg(_config.outputfile(), svg::Layout(dimensions, svg::Layout::TopLeft));

    PenStroke gpt = PenStroke::createPenStroke(_config.penconfig(), _box);

    Ribbon& ribbon = load.ribList().at(0);
    for (Polyline& line : ribbon.lines()) {
        line.removeConsecutiveDuplicatePoints(kSvgCollapseEpsilon);
    }

    for (const Polyline& line : ribbon.lines()) {
        Polyline smoothedPolyline = Smoothing::getCurveSmoothingChaikin(
            line, _config.smoothing_tension(),
            static_cast<uint32_t>(_config.smoothing_iterations()));
        gpt.createStroke(smoothedPolyline);
    }

    for (std::size_t i = 0; i < ribbon.lines().size(); ++i) {
        Polyline& polyline = ribbon.lines().at(i);
        polyline.id = static_cast<int32_t>(i);
    }

    Arrangement_2 arr = ribbon.createArr();
    // END DEBUG
    std::unordered_set<const Face*> faceSet;

    std::vector<const Face*> faceList;

    for (const Halfedge& halfedge : RangeHelper::make(arr.edges_begin(), arr.edges_end())) {
        if (halfedge.source()->point() == halfedge.curve().source()) {
            if (faceSet.emplace(&*halfedge.face()).second) {
                faceList.emplace_back(&*halfedge.face());
            }
        } else {
            if (faceSet.emplace(&*halfedge.twin()->face()).second) {
                faceList.emplace_back(&*halfedge.twin()->face());
            }
        }
    }

    std::cout << "faceList " << faceList.size() << " arr " << arr.number_of_faces() << '\n';

    for (const Face* face : faceList) {
        svg::Path path(svg::Color(kDebugFaceRed, kDebugFaceGreen, kDebugFaceBlue),
                       svg::Color::Transparent);
        gpt.drawFace(path, *face);
        docSvg << path;
    }

    using laby::basic::Color;
    const auto fillColorValue = static_cast<uint32_t>(ribbon.fill_color());
    svg::Path path(svg::Color(static_cast<int32_t>(Color::get_red(fillColorValue)),
                              static_cast<int32_t>(Color::get_green(fillColorValue)),
                              static_cast<int32_t>(Color::get_blue(fillColorValue))),
                   svg::Color::Transparent);

    gpt.drawOutline(path);
    docSvg << path;

    docSvg.save();
}

void GraphicRendering::printRibbonSvg(const CGAL::Bbox_2& bbox, const std::string& filename,
                                      const double& thickness,
                                      const std::vector<Ribbon>& ribbonList) {

    // if std::io error problem, check if output directory exists !!
    svg::Dimensions dimensions(bbox.xmax(), bbox.ymax());
    svg::DocumentSVG docSvg(filename, svg::Layout(dimensions, svg::Layout::TopLeft));

    for (const Ribbon& ribbon : ribbonList) {
        Ribbon cleanRibbon = ribbon;
        for (Polyline& polyline : cleanRibbon.lines()) {
            polyline.removeConsecutiveDuplicatePoints(kSvgCollapseEpsilon);
        }

        using laby::basic::Color;
        const auto fillColorValue = static_cast<uint32_t>(cleanRibbon.fill_color());
        svg::Stroke stroke(thickness,
                           svg::Color(static_cast<int32_t>(Color::get_red(fillColorValue)),
                                      static_cast<int32_t>(Color::get_green(fillColorValue)),
                                      static_cast<int32_t>(Color::get_blue(fillColorValue))));

        svg::Path path(svg::Color::Transparent, stroke);

        PenStroke::drawRibbonStroke(path, cleanRibbon);

        docSvg << path;
    }
    docSvg.save();
}

} /* namespace laby */
