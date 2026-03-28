/*
 * GraphicRendering.cpp
 *
 *  Created on: Feb 21, 2018
 *      Author: florian
 */

#include "GraphicRendering.h"

#include <CGAL/Bbox_2.h>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>

#include "PenStroke.h"
#include "../Polyline.h"
#include "../Ribbon.h"
#include "../SVGParser/Loader.h"
#include "../SVGWriter/DocumentSVG.h"
#include "../Smoothing.h"
#include "../basic/Color.h"
#include "../protoc/AllConfig.pb.h"

namespace laby {

namespace {
constexpr double kSvgCollapseEpsilon = 1e-12;
constexpr int32_t kDebugFaceRed = 250;
constexpr int32_t kDebugFaceGreen = 10;
constexpr int32_t kDebugFaceBlue = 10;
} // namespace

GraphicRendering::GraphicRendering(proto::GraphicRendering config) : _config(std::move(config)) {

    const svgp::Loader load(_config.inputfile());

    _box = CGAL::Bbox_2(0, 0, load.viewBox().xmax(), load.viewBox().ymax());
    const svg::Dimensions dimensions(svg::Dimensions::Size{_box.xmax(), _box.ymax()});
    svg::DocumentSVG docSvg(_config.outputfile(), svg::Layout(dimensions, svg::Layout::Origin::TopLeft));

    PenStroke gpt = PenStroke::createPenStroke(_config.penconfig(), _box);

    Ribbon ribbon = load.ribList().at(0);
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
        polyline.setId(static_cast<int32_t>(i));
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
        svg::Path path(svg::Fill(svg::Color(svg::Color::Rgb{kDebugFaceRed, kDebugFaceGreen, kDebugFaceBlue})));
        gpt.drawFace(path, *face);
        docSvg << path;
    }

    using laby::basic::Color;
    const auto fillColorValue = static_cast<uint32_t>(ribbon.fillColor());
    svg::Path path(svg::Fill(svg::Color(svg::Color::Rgb{
        static_cast<int32_t>(Color::getRed(fillColorValue)),
        static_cast<int32_t>(Color::getGreen(fillColorValue)),
        static_cast<int32_t>(Color::getBlue(fillColorValue))})));

    gpt.drawOutline(path);
    docSvg << path;

    static_cast<void>(docSvg.save());
}

void GraphicRendering::printRibbonSvg(const CGAL::Bbox_2& bbox, const std::string& filename,
                                      const double& thickness,
                                      const std::vector<Ribbon>& ribbonList) {

    // if std::io error problem, check if output directory exists !!
    const svg::Dimensions dimensions(svg::Dimensions::Size{bbox.xmax(), bbox.ymax()});
    svg::DocumentSVG docSvg(filename, svg::Layout(dimensions, svg::Layout::Origin::TopLeft));

    for (const Ribbon& ribbon : ribbonList) {
        Ribbon cleanRibbon = ribbon;
        for (Polyline& polyline : cleanRibbon.lines()) {
            polyline.removeConsecutiveDuplicatePoints(kSvgCollapseEpsilon);
        }

        using laby::basic::Color;
        const auto fillColorValue = static_cast<uint32_t>(cleanRibbon.fillColor());
        svg::Stroke stroke(thickness,
                           svg::Color(svg::Color::Rgb{
                               static_cast<int32_t>(Color::getRed(fillColorValue)),
                               static_cast<int32_t>(Color::getGreen(fillColorValue)),
                               static_cast<int32_t>(Color::getBlue(fillColorValue))}));

        svg::Path path(svg::Fill(), stroke);

        PenStroke::drawRibbonStroke(path, cleanRibbon);

        docSvg << path;
    }
    static_cast<void>(docSvg.save());
}

} /* namespace laby */
