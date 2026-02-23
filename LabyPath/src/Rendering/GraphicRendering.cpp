/*
 * GraphicRendering.cpp
 *
 *  Created on: Feb 21, 2018
 *      Author: florian
 */

#include "GraphicRendering.h"

#include <cstdint>
#include <CGAL/Bbox_2.h>
#include <svgpp/factory/integer_color.hpp>
#include <iostream>
#include <string>

#include "../basic/Color.h"
#include "../GeomData.h"
#include "../Smoothing.h"
#include "../OrientedRibbon.h"
#include "../SVGParser/Loader.h"

namespace laby {

GraphicRendering::GraphicRendering(const proto::GraphicRendering& config) :
        _config(config) {

    svgp::Loader load(config.inputfile());

    _box = CGAL::Bbox_2(0, 0, load.viewBox().xmax(), load.viewBox().ymax());
    svg::Dimensions dimensions(_box.xmax(), _box.ymax());
    svg::DocumentSVG docSvg(config.outputfile(), svg::Layout(dimensions, svg::Layout::TopLeft));

    PenStroke gpt = PenStroke::createPenStroke(_config.penconfig(), _box);

    Ribbon& ribbon = load.ribList().at(0);

    for (const Polyline& line : ribbon.lines()) {
        Polyline pl = Smoothing::getCurveSmoothingChaikin(line, _config.smoothing_tension(), static_cast<uint32_t>(_config.smoothing_iterations()));
        gpt.createStroke(pl);

    }

    for (std::size_t i = 0; i < ribbon.lines().size(); ++i) {
        Polyline& pl = ribbon.lines().at(i);
        pl.number = static_cast<int32_t>(i);
    }

    Arrangement_2 arr = ribbon.createArr();
    //END DEBUG
    std::unordered_set<const Face*> faceSet;

    std::vector<const Face*> faceList;

    for (const Halfedge& he : RangeHelper::make(arr.edges_begin(), arr.edges_end())) {
        if (he.source()->point() == he.curve().source()) {
            if (faceSet.emplace(&*he.face()).second) {
                faceList.emplace_back(&*he.face());
            }
        } else {
            if (faceSet.emplace(&*he.twin()->face()).second) {
                faceList.emplace_back(&*he.twin()->face());
            }
        }
    }

    std::cout << "faceList " << faceList.size() << " arr " << arr.number_of_faces() << std::endl;

    for (const Face* fc : faceList) {
        svg::Path path(svg::Color(250, 10, 10), svg::Color::Transparent);
        gpt.drawFace(path, *fc);
        docSvg << path;
    }

    using laby::basic::Color;
    const uint32_t c = static_cast<uint32_t>(ribbon.fill_color());
    svg::Path path(svg::Color(static_cast<int32_t>(Color::get_red(c)), static_cast<int32_t>(Color::get_green(c)), static_cast<int32_t>(Color::get_blue(c))), svg::Color::Transparent);

    gpt.draw_outline(path);
    docSvg << path;

    docSvg.save();

}

void GraphicRendering::printRibbonSvg(const CGAL::Bbox_2& bbox, const std::string& filename, const double& thickness, const std::vector<Ribbon>& ribbonList) {

    // if std::io error problem, check if output directory exists !!
    svg::Dimensions dimensions(bbox.xmax(), bbox.ymax());
    svg::DocumentSVG docSvg(filename, svg::Layout(dimensions, svg::Layout::TopLeft));

    for (const Ribbon& ribbon : ribbonList) {

        using laby::basic::Color;
        const uint32_t c = static_cast<uint32_t>(ribbon.fill_color());
        svg::Stroke stroke(thickness, svg::Color(static_cast<int32_t>(Color::get_red(c)), static_cast<int32_t>(Color::get_green(c)), static_cast<int32_t>(Color::get_blue(c))));

        svg::Path path(svg::Color::Transparent, stroke);

        PenStroke::drawRibbonStroke(path, ribbon);

        docSvg << path;
    }
    docSvg.save();
}

} /* namespace laby */
