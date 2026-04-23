/*
 * PathRenderingTest.cpp
 *
 *  Created on: Mar 16, 2018
 *      Author: florian
 */

#include "PathRenderingTest.h"

#include <CGAL/Bbox_2.h>
#include <cmath>
#include <cstddef>

#include "../Anisotrop/Cell.h"
#include "../Anisotrop/Placement.h"
#include "../Rendering/Fenetre.h"
#include "../Rendering/GraphicRendering.h"
#include "../Rendering/PenStroke.h"
#include "../generator/StreamLine.h"

namespace laby {
namespace aniso {

generator::StreamLine PathRenderingTest::createStreamLine() {
    generator::StreamLine::Config config_st;
    config_st.divisor = 2;
    config_st.size = 320;
    generator::StreamLine st(config_st);
    st.drawSpiral({{80, 80}, 100, M_PI / 40.});
    st.drawSpiral({{40., 40.}, 32, -M_PI / 4.});
    st.drawSpiral({{220., 200.}, 76, -M_PI / 4.});
    st.render();
    return st;
}

aniso::Cell PathRenderingTest::createCell() {
    aniso::Cell cell;

    generator::StreamLine st = createStreamLine();
    st.addToArrangement(cell.arr());
    // createArr
    //  global insertion of all segments
    //  insertion of additional segment to link random vertices to the present lattice
    cell.drawRectOutline({60., 60., 100, 100.}, RectOutlineConfig{10, 10, 60});
    cell.drawRectOutline({180., 160., 260., 240.}, RectOutlineConfig{10, 2.2, 100});
    const CGAL::Bbox_2 bbox{0., 0., 320., 320.};
    cell.shuffleVertices();
    std::size_t maxPin = 200; // 200;
    cell.createRandomPin(bbox, maxPin);
    return cell;
}

int PathRenderingTest::test(int argc, char* argv[]) {

    generator::StreamLine st = createStreamLine();

    // aniso::Cell cell = createCell();

    //    GeomFeatures geoFeature;
    //    geoFeature.arr() = cell.arr();
    // geoFeature.setNets(aniso::Net::extractPins(cell.nets()));

    GraphicRendering::Config render_config;
    render_config.fenetre.height_point = 680;
    render_config.fenetre.width_point = 680;
    render_config.fenetre.screen_width_pixel = 1000;
    render_config.fenetre.screen_heigth_pixel = 1000;

    render_config.fenetre.surface = Fenetre::Config::SVG;
    render_config.fenetre.interactive_display = true;
    render_config.zoom = 2.;
    double factorPen = 0.2;
    render_config.emulate_pencil = false;
    render_config.penConfig.thickness = 1 * factorPen;
    render_config.penConfig.antisymmetric_amplitude = 2 * factorPen;
    render_config.penConfig.symmetric_amplitude = 0.7 * factorPen;
    render_config.penConfig.antisymmetric_freq = 10;
    render_config.penConfig.symmetric_freq = 3;
    render_config.penConfig.canvas_size = 1000;
    // render_config.thickness = 0.5;

    GraphicRendering render(render_config);

    std::vector<Ribbon> result;
    result.emplace_back(st.circularList());
    result.emplace_back(st.radialList());

    return render.render(result, argc, argv);
}
} /* namespace aniso */
} /* namespace laby */
