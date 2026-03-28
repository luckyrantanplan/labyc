/*
 * Smoothing.cpp
 *
 *  Created on: May 28, 2018
 *      Author: florian
 */

#include "Smoothing.h"
#include "Polyline.h"
#include <cstdint>
#include <algorithm>
#include <cstddef>
#include <CGAL/Kernel/global_functions_2.h>

namespace laby {

auto Smoothing::getCurveSmoothingChaikin(const Polyline& line, double tension, uint32_t nrOfIterations) -> Polyline {
    //checks
    if (line.points.size() < 3) {
        return line;
    }

    tension = std::max(tension, 0.);
    // 'the tension factor defines a scale between corner cutting distance in segment half length,
    //   'i.e. between 0.05 and 0.45. The opposite corner will be cut by the inverse
    // '(i.e. 1-cutting distance) to keep symmetry.
    // 'with a tension value of 0.5 this amounts to 0.25 = 1/4 and 0.75 = 3/4,
    // 'the original Chaikin values
    double const cutdist = 0.05 + (tension * 0.4);
    //  'make a copy of the pointlist and iterate it
    Polyline nl = line;
    for (uint32_t i = 0; i < nrOfIterations; ++i) {
        nl = getSmootherChaikin(nl, cutdist);
    }
    return nl;
}
auto Smoothing::getSmootherChaikin(const Polyline& line, double cuttingDist) -> Polyline {
    Polyline nl(line.id);
    nl.closed = line.closed;
    if (line.points.at(0) != line.points.back() or !line.closed) {
        nl.points.emplace_back(line.points.at(0));
        chaikinPointCompute(line, cuttingDist, nl);
        //'always add the last point
        nl.points.emplace_back(line.points.back());
    } else {
        chaikinPointCompute(line, cuttingDist, nl);
        nl.points.emplace_back(nl.points.at(0));
    }
    return nl;
}

void Smoothing::chaikinPointCompute(const Polyline& line, double cuttingDist, Polyline& nl) {
    for (std::size_t i = 1; i < line.points.size(); ++i) {
        nl.points.emplace_back(CGAL::barycenter(line.points.at(i - 1), 1 - cuttingDist, line.points.at(i)));
        nl.points.emplace_back(CGAL::barycenter(line.points.at(i - 1), cuttingDist, line.points.at(i)));
    }
}
} /* namespace laby */
