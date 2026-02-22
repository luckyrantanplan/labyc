/*
 * GrayPenTest.cpp
 *
 *  Created on: Jun 11, 2018
 *      Author: florian
 */

#include "BezierTest.h"

#include <cairomm/context.h>
#include <cairomm/refptr.h>
#include <vector>

#include "../agg/agg_curves.h"
//#include "../generator/HqNoise.h"
#include "../GeomData.h"
#include "../Rendering/Fenetre.h"
#include "../Rendering/PenStroke.h"

namespace laby {

int BezierTest::test(int argc, char *argv[]) {

    {
        agg::Curve3 curve(1, 1, 50, 100, 100, 1);

        const std::vector<CGAL::Point_2<Kernel> >& points = curve.getPoints();

        std::cout << points << std::endl;

    }
    {
        agg::Curve4 curve(1, 1, 1, 100, 100, 100, 100, 1);

        std::cout << curve.getPoints() << std::endl;

    }
    exit(0);
    Polyline pl;
    pl.points.emplace_back(600, 600);
    pl.points.emplace_back(600, 200);
    pl.points.emplace_back(20, 200);
    pl.points.emplace_back(500, 500);
    pl.points.emplace_back(400, 600);

    Fenetre::Config confFenetre;

    confFenetre.height_point = 1000;
    confFenetre.width_point = 1000;
    confFenetre.screen_width_pixel = 1000;
    confFenetre.screen_heigth_pixel = 1000;

    return Fenetre::main(argc, argv, [& ](const Cairo::RefPtr<Cairo::Context>& cr) {

        cr->set_line_width(1);

        cr->scale(1., 1.);
        PenStroke::Config penConfig;
        penConfig.thickness=4;
        penConfig.antisymmetric_amplitude=10;
        penConfig.symmetric_amplitude=3;
        penConfig.antisymmetric_freq=200;
        penConfig.symmetric_freq=20;
        penConfig.canvas_size=1000;
        PenStroke gpt=PenStroke::createPenStroke(penConfig);
        gpt.drawPolylineFill(cr ,pl);
        return true;
    }, confFenetre);
}

} /* namespace laby */
