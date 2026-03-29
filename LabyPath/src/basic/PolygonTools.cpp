/*
 * PolygonTools.cpp
 *
 *  Created on: Mar 8, 2018
 *      Author: florian
 */

#include "PolygonTools.h"
#include "GeomData.h"

#include <CGAL/Kernel/global_functions_2.h>
#include <CGAL/Vector_2.h>
#include <CGAL/enum.h>
#include <CGAL/number_utils.h>
#include <basic/RangeHelper.h>
#include <cmath>
#include <iostream>
#include <ostream>

namespace laby {

auto PolygonTools::makeTrapeze(const Point_2& sourcePoint, const Point_2& targetPoint,
                               const double& thickness1,
                               const double& thickness2) -> Linear_polygon {

    Linear_polygon polygon;
    makeTrapeze(polygon, sourcePoint, targetPoint, thickness1, thickness2);

    return polygon;
}

void PolygonTools::makeTrapeze(Linear_polygon& polygon, const Point_2& sourcePoint,
                               const Point_2& targetPoint, const double& thickness1,
                               const double& thickness2) {
    CGAL::Vector_2<Kernel> const direction(sourcePoint, targetPoint);
    CGAL::Vector_2<Kernel> const perpendicular = direction.perpendicular(CGAL::LEFT_TURN);

    double const scaleFactor = 0.5 / std::sqrt(CGAL::to_double(perpendicular.squared_length()));

    double const firstOffsetLength = thickness1 * scaleFactor;
    CGAL::Vector_2<Kernel> const firstOffset = perpendicular * firstOffsetLength;

    polygon.push_back(sourcePoint + firstOffset);
    polygon.push_back(sourcePoint - firstOffset);

    double const secondOffsetLength = thickness2 * scaleFactor;
    CGAL::Vector_2<Kernel> const secondOffset = perpendicular * secondOffsetLength;
    polygon.push_back(targetPoint - secondOffset);
    polygon.push_back(targetPoint + secondOffset);
}

void PolygonTools::insertPointPolygon(const Point_2& firstCurrentPoint,
                                      const Point_2& secondCurrentPoint,
                                      const Point_2& firstExtensionPoint,
                                      const Point_2& secondExtensionPoint,
                                      Linear_polygon& polygon) {
    Kernel::Orientation const orientation =
        CGAL::orientation(firstCurrentPoint, secondCurrentPoint, firstExtensionPoint);
    if (orientation == CGAL::RIGHT_TURN) {
        if (CGAL::compare_squared_distance(secondCurrentPoint, firstExtensionPoint, 0) ==
            CGAL::LARGER) {
            polygon.insert(polygon.vertices_begin() + 3, firstExtensionPoint);
        }
    } else {
        if (orientation == CGAL::LEFT_TURN) {
            if (CGAL::compare_squared_distance(firstCurrentPoint, secondExtensionPoint, 0) ==
                CGAL::LARGER) {
                polygon.insert(polygon.vertices_begin() + 3, secondExtensionPoint);
            }
        }
    }
}

void PolygonTools::extendPolygon(Linear_polygon& polygon, const Linear_polygon& extensionPolygon) {
    const Point_2& firstExtensionVertex = extensionPolygon.vertex(0);
    const Point_2& secondExtensionVertex = extensionPolygon.vertex(1);

    const Point_2& secondCurrentVertex = polygon.vertex(3);
    const Point_2& firstCurrentVertex = polygon.vertex(2);

    insertPointPolygon(firstCurrentVertex, secondCurrentVertex, firstExtensionVertex,
                       secondExtensionVertex, polygon);
}

auto PolygonTools::getSegmentContainingPoint(const Linear_polygon& polygon,
                                             const Point_2& center) -> Linear_polygon::Segment_2 {
    for (const Linear_polygon::Segment_2& segment :
         RangeHelper::make(polygon.edges_begin(), polygon.edges_end())) {
        if (segment.has_on(center)) {
            return segment;
        }
    }
    std::cout << "ERROR seg of poly " << polygon << " does not have center point " << center
              << '\n';
    return {};
}

auto PolygonTools::createJoinTriangle(const Linear_polygon& firstPolygon,
                                      const Linear_polygon& secondPolygon,
                                      const Point_2& center) -> Linear_polygon {
    Linear_polygon joinTriangle;

    const Linear_polygon::Segment_2 firstSegment = getSegmentContainingPoint(firstPolygon, center);
    const Linear_polygon::Segment_2 secondSegment =
        getSegmentContainingPoint(secondPolygon, center);

    Kernel::Orientation const orientation =
        CGAL::orientation(firstSegment.source(), secondSegment.source(), secondSegment.target());

    if (orientation == CGAL::LEFT_TURN) {

        joinTriangle.push_back(firstSegment.target());
        joinTriangle.push_back(center);
        joinTriangle.push_back(secondSegment.source());

    } else if (orientation == CGAL::RIGHT_TURN) {
        joinTriangle.push_back(secondSegment.target());
        joinTriangle.push_back(center);
        joinTriangle.push_back(firstSegment.source());
    }

    return joinTriangle;
}

} /* namespace laby */
