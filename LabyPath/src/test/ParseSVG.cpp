/*
 * ParseSVG.cpp
 *
 *  Created on: Jun 14, 2018
 *      Author: florian
 */

#include "ParseSVG.h"

#include <CGAL/Bbox_2.h>
#include <iostream>
#include <vector>

#include "../Anisotrop/Cell.h"
#include "../Anisotrop/Placement.h"
#include "../Anisotrop/Routing.h"
#include "../ConfigAll.h"
#include "../flatteningOverlap/PathRendering.h"
#include "../GeomFeatures.h"
#include "../PolyConvex.h"
#include "../Rendering/Fenetre.h"
#include "../Rendering/GraphicRendering.h"
#include "../Rendering/PenStroke.h"
#include "../Ribbon.h"
#include "../SkeletonGrid.h"
#include "../SVGParser/Loader.h"

namespace laby {

aniso::Cell ParseSVG::createCell(const ConfigAll& config) {

    std::vector<Ribbon> vRibbons = svgp::Loader::load(config.filename);
    for (Ribbon& rib : vRibbons) {
        rib.simplify(config.simplificationOfOriginalSVG);
    }

    SkeletonGrid skeletonGrid(config.skeleton_config);
    skeletonGrid.create(vRibbons);

    aniso::Cell cell(config.cellConf, skeletonGrid.bbox(), skeletonGrid.segResult());

    cell.completePin();
    return cell;
}

void ParseSVG::route(const ConfigAll& config, aniso::Cell& cell, GeomFeatures& geoFeature) {

    aniso::Placement place(config.place_config);
    std::vector<PolyConvex> polyConvexList = place.place(cell);
    // geoFeature.setNets(aniso::Net::extractPins(cell.nets()));
    PathRendering::pathRender(polyConvexList, geoFeature.arr());
}

int ParseSVG::test(int argc, char *argv[]) {

    std::cout << "Start "<< argc << std::endl;

    ConfigAll config;
    if (argc < 2) {
        std::cout << "conf missing" << std::endl;
    }
    std::cout << argv[1] << std::endl;
    std::string filename(argv[1]);
    std::cout << filename << std::endl;
    config.decodeProtobuf(filename);

//    config.print();
//    exit(0);
    //render_config.thickness = 0.5;

    aniso::Cell cell = createCell(config);
    GeomFeatures geoFeature;
    // geoFeature.arr() = cell.arr();

    route(config, cell, geoFeature);
    std::cout << "finish arrangement creation" << std::endl;
    GraphicRendering::Config& render_config = config.render_config;
    render_config.fenetre.height_point = (cell.bbox().ymax() + render_config.translate) * render_config.zoom;
    render_config.fenetre.width_point = (cell.bbox().xmax() + render_config.translate) * render_config.zoom;
    render_config.penConfig.canvas_size = render_config.fenetre.height_point + render_config.fenetre.width_point;

    GraphicRendering render(render_config);
    return render.render(geoFeature, argc, argv);
}

} /* namespace laby */
