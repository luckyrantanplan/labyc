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
    static auto makeTrapeze(const Point_2& a, const Point_2& b, const double& thickness1, const double& thickness2) -> Linear_polygon;
    static void makeTrapeze(Linear_polygon& poly, const Point_2& a, const Point_2& b, const double& thickness1, const double& thickness2);
    static void extendPolygon(Linear_polygon& p1, const Linear_polygon& p2);
    static auto getSegmentContainingPoint(const Linear_polygon& p1, const Point_2& center) -> const Linear_polygon::Segment_2;

    static auto createJoinTriangle(const Linear_polygon& p1, const Linear_polygon& p2, const Point_2& center) -> Linear_polygon;
    static void insertPointPolygon(const Point_2& a2, const Point_2& a3, const Point_2& b0, const Point_2& b1, Linear_polygon& p1);
};

} /* namespace laby */

#endif /* BASIC_POLYGONTOOLS_H_ */
