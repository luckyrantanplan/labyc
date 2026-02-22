/*
 * GraphicRendering.h
 *
 *  Created on: Feb 21, 2018
 *      Author: florian
 */

#ifndef GRAPHICRENDERING_H_
#define GRAPHICRENDERING_H_

//#include <cstdint>

#include <complex>
#include <vector>

#include "../Ribbon.h"
#include "PenStroke.h"
#include "../protoc/AllConfig.pb.h"

namespace laby {

class GraphicRendering {
public:

    GraphicRendering(const proto::GraphicRendering& config);

    static void printRibbonSvg(const CGAL::Bbox_2& bbox, const std::string& filename, const double& thickness, const std::vector<Ribbon>& ribbonList);

private:

    const proto::GraphicRendering _config;
    CGAL::Bbox_2 _box;
};

} /* namespace laby */

#endif /* GRAPHICRENDERING_H_ */
