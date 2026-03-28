/*
 * Smoothing.h
 *
 *  Created on: May 28, 2018
 *      Author: florian
 */

#ifndef SMOOTHING_H_
#define SMOOTHING_H_

#include <cstdint>

#include "Polyline.h"

namespace laby {

class Smoothing {
public:
    static auto getCurveSmoothingChaikin(const Polyline& line, double tension, uint32_t nrOfIterations) -> Polyline;

    static auto getSmootherChaikin(const Polyline& line, double cuttingDist) -> Polyline;

private:
    static void chaikinPointCompute(const Polyline& line, double cuttingDist, Polyline& nl);
};

} /* namespace laby */

#endif /* SMOOTHING_H_ */
