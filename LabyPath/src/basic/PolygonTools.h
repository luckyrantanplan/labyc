/*
 * PolygonTools.h
 *
 *  Created on: Mar 8, 2018
 *      Author: florian
 */

#ifndef BASIC_POLYGONTOOLS_H_
#define BASIC_POLYGONTOOLS_H_

#include <CGAL/Polygon_2.h>

#include "../GeomData.h"

namespace laby {

using Linear_polygon = CGAL::Polygon_2<Kernel>;

class PolygonTools {
  public:
    static auto makeTrapeze(const Point_2& sourcePoint, const Point_2& targetPoint,
                            const double& thickness1, const double& thickness2) -> Linear_polygon;
    static void makeTrapeze(Linear_polygon& polygon, const Point_2& sourcePoint,
                            const Point_2& targetPoint, const double& thickness1,
                            const double& thickness2);
    static void extendPolygon(Linear_polygon& polygon, const Linear_polygon& extensionPolygon);
    static auto getSegmentContainingPoint(const Linear_polygon& polygon,
                                          const Point_2& center) -> Linear_polygon::Segment_2;

    static auto createJoinTriangle(const Linear_polygon& firstPolygon,
                                   const Linear_polygon& secondPolygon,
                                   const Point_2& center) -> Linear_polygon;
    static void insertPointPolygon(const Point_2& firstCurrentPoint,
                                   const Point_2& secondCurrentPoint,
                                   const Point_2& firstExtensionPoint,
                                   const Point_2& secondExtensionPoint, Linear_polygon& polygon);
};

} /* namespace laby */

#endif /* BASIC_POLYGONTOOLS_H_ */
