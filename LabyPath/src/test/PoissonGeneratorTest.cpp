/*
 * PoissonGeneratorTest.cpp
 *
 *  Created on: May 11, 2018
 *      Author: florian
 */

#include "../generator/PoissonGenerator.h"
#include "PoissonGeneratorTest.h"

#include <CGAL/Bbox_2.h>
#include <easy/profiler.h>
#include <complex>
#include <iostream>
#include <vector>

#include "../GeomFeatures.h"

namespace laby {

GeomFeatures PoissonGeneratorTest::test() {
    GeomFeatures gf;

    const CGAL::Bbox_2 bbox { 0., 0., 3200., 3200. };
    std::vector<std::complex<double>> Points;
    {
        EASY_BLOCK("PoissonGeneratorTest");
        Points = generator::PoissonPoints::generateRectangle(bbox, 100000);
    }
    for (std::complex<double>& sp : Points) {
        // std::cout << sp.real() << " ; " << sp.imag() << std::endl;
        gf.nets().emplace_back(sp);
    }
    std::cout << "Points size " << Points.size() << std::endl;
    return gf;
}
} /* namespace laby */
