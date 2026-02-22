/*
 * PolygonSetTest.cpp
 *
 *  Created on: Jun 20, 2018
 *      Author: florian
 */

#include "PolygonSetTest.h"

#include <CGAL/Arrangement_2/Arrangement_on_surface_2_global.h>
#include <CGAL/Arrangement_on_surface_with_history_2.h>
#include <CGAL/Point_2.h>
#include <iostream>
#include <vector>

#include "../generator/StreamLine.h"
#include "../GeomData.h"
#include "../GeomFeatures.h"
#include "../Rendering/Fenetre.h"
#include "../Rendering/GraphicRendering.h"
#include "../Rendering/PenStroke.h"
#include "../SVGShapeToGrid.h"

namespace laby {

int PolygonSetTest::test(int argc, char* argv[]) {
    SVGShapeToGrid::Arr_with_hist_2 arr;
    // Insert s1, s2 and s3 incrementally:
    std::vector<Segment_2> segs;
    segs.emplace_back(Point_2(0, 0), Point_2(100, 0));
    segs.emplace_back(Point_2(100, 0), Point_2(100, 100));
    segs.emplace_back(Point_2(100, 100), Point_2(0, 100));
    segs.emplace_back(Point_2(0, 100), Point_2(0, 0));
//    segs.emplace_back(Point_2(5, 5), Point_2(5, 15));
//    segs.emplace_back(Point_2(5, 15), Point_2(15, 15));
//    segs.emplace_back(Point_2(15, 15), Point_2(15, 5));
//    segs.emplace_back(Point_2(15, 5), Point_2(5, 5));

//    segs.emplace_back(Point_2(5, 5), Point_2(5, 10));
//    segs.emplace_back(Point_2(5, 10), Point_2(10, 10));
//    segs.emplace_back(Point_2(10, 10), Point_2(10, 5));
//    segs.emplace_back(Point_2(10, 5), Point_2(5, 5));

    segs.emplace_back(Point_2(50, 0), Point_2(0, 50));
    segs.emplace_back(Point_2(0, 50), Point_2(50, 100));
    segs.emplace_back(Point_2(50, 100), Point_2(100, 50));
    segs.emplace_back(Point_2(100, 50), Point_2(50, 0));

    CGAL::insert(arr, segs.begin(), segs.end());

    std::vector<Segment_2> segResult = SVGShapeToGrid::get_grid(arr);

    generator::StreamLine::Config config_st;
    config_st.divisor = 1;
    config_st.resolution = 4.; //like the zoom

    // Ribbon r = generator::StreamLine::getRadial(config_st, segResult);
    GeomFeatures geoFeature;
    //  geoFeature.arr() = r.createArr();

    // geoFeature.setNets(aniso::Net::extractPins(cell.nets()));
    CGAL::insert(geoFeature.arr(), segResult.begin(), segResult.end());

    std::cout << "finish arrangement creation" << std::endl;
    GraphicRendering::Config render_config;
    render_config.fenetre.height_point = 680;
    render_config.fenetre.width_point = 680;
    render_config.fenetre.screen_width_pixel = 1000;
    render_config.fenetre.screen_heigth_pixel = 1000;
    render_config.fenetre.surface = Fenetre::Config::SVG;
    render_config.fenetre.interactive_display = true;
    render_config.zoom = 4.;
    render_config.emulate_pencil = 0;
    render_config.penConfig.thickness = 1. / 6.;
    render_config.penConfig.antisymmetric_amplitude = 0;
    render_config.penConfig.symmetric_amplitude = 0;
    render_config.penConfig.antisymmetric_freq = 10;
    render_config.penConfig.symmetric_freq = 3;
    render_config.penConfig.canvas_size = 1000;
    //render_config.thickness = 0.5;
    GraphicRendering render(render_config);
    return render.render(geoFeature, argc, argv);
}
} /* namespace laby */
