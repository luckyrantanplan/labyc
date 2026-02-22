/*
 * Smoothing.h
 *
 *  Created on: May 28, 2018
 *      Author: florian
 */

#ifndef SMOOTHING_H_
#define SMOOTHING_H_

#include <bits/stdint-uintn.h>

#include "Polyline.h"

namespace laby {

class Smoothing {
public:
    static Polyline getCurveSmoothingChaikin(const Polyline& line, double tension, uint32_t nrOfIterations);

    static Polyline getSmootherChaikin(const Polyline& line, double cuttingDist);

private:
    static void chaitKinPointCompute(const Polyline& line, double cuttingDist, Polyline& nl);
};

} /* namespace laby */

#endif /* SMOOTHING_H_ */
