/*
 * ShapeContext2.cpp
 *
 *  Created on: Jun 18, 2018
 *      Author: florian
 */

#include "ShapeContext.h"

#include <CGAL/number_utils.h>
#include <iostream>
#include <svgpp/definitions.hpp>
#include <svgpp/policy/marker_events.hpp>
#include <variant>

#include "../agg/agg_curves.h"
#include "Ribbon.h"
#include "SVGParser/Context.h"
#include "SVGParser/Stylable.h"

namespace laby::svgp {

ShapeContext::ShapeContext(BaseContext& parent) : BaseContext(parent) {
    getRibbon().emplace_back();
}

auto ShapeContext::currentRibbon() -> Ribbon& {
    return getRibbon().back();
}

// SVG++ discovers these callbacks by exact name.

void ShapeContext::path_move_to(double xCoordinate, double yCoordinate,
                                svgpp::tag::coordinate::absolute coordinateTag) {
    static_cast<void>(coordinateTag);

    if (currentRibbon().lines().empty() or !currentRibbon().lines().back().points().empty()) {
        currentRibbon().lines().emplace_back();
    }

    currentRibbon().lines().back().points().emplace_back(xCoordinate, yCoordinate);
}

void ShapeContext::path_line_to(double xCoordinate, double yCoordinate,
                                svgpp::tag::coordinate::absolute coordinateTag) {
    static_cast<void>(coordinateTag);
    currentRibbon().lines().back().points().emplace_back(xCoordinate, yCoordinate);
}

void ShapeContext::path_cubic_bezier_to(double xControl1, double yControl1, double xControl2,
                                        double yControl2, double xCoordinate, double yCoordinate,
                                        svgpp::tag::coordinate::absolute coordinateTag) {
    static_cast<void>(coordinateTag);

    {
        Point_2& ref = currentRibbon().lines().back().points().back();
        agg::Curve4 curve(CGAL::to_double(ref.x()), CGAL::to_double(ref.y()), xControl1, yControl1,
                          xControl2, yControl2, xCoordinate, yCoordinate);

        const auto& vect = curve.getPoints();
        auto& ribbonLine = currentRibbon().lines().back().points();
        // remove last item
        ribbonLine.insert(ribbonLine.end(), ++vect.begin(), vect.end());
    }
}

void ShapeContext::path_quadratic_bezier_to(double xControl, double yControl, double xCoordinate,
                                            double yCoordinate,
                                            svgpp::tag::coordinate::absolute coordinateTag) {
    static_cast<void>(coordinateTag);

    Point_2& ref = currentRibbon().lines().back().points().back();
    agg::Curve3 curve(CGAL::to_double(ref.x()), CGAL::to_double(ref.y()), xControl, yControl,
                      xCoordinate, yCoordinate);

    const auto& vect = curve.getPoints();
    auto& ribbonLine = currentRibbon().lines().back().points();
    // remove last item
    ribbonLine.insert(ribbonLine.end(), ++vect.begin(), vect.end());
}

void ShapeContext::path_elliptical_arc_to(double /*radiusX*/, double /*radiusY*/,
                                          double /*xAxisRotation*/, bool /*largeArcFlag*/,
                                          bool /*sweepFlag*/, double xCoordinate,
                                          double yCoordinate,
                                          svgpp::tag::coordinate::absolute coordinateTag) {
    static_cast<void>(coordinateTag);
    std::cout << "path_elliptical_arc_to " << xCoordinate << " " << yCoordinate << '\n';
}

void ShapeContext::path_close_subpath() {
    currentRibbon().lines().back().setClosed(true);
    std::vector<Point_2>& points = currentRibbon().lines().back().points();
    if (points.back() != points.front()) {
        points.emplace_back(points.front());
    }
    currentRibbon().lines().emplace_back();
}

auto ShapeContext::getColor(const Paint& paint) -> color_t {
    EffectivePaint epaint = style().getEffectivePaint(paint);
    if (const color_t* color = std::get_if<color_t>(&epaint)) {
        return *color;
    }
    return 0;
}

void ShapeContext::path_exit() {
    currentRibbon().setFillColor(getColor(style().fillPaint()));
    currentRibbon().setStrokeColor(getColor(style().strokePaint()));
    currentRibbon().setStrokeWidth(style().strokeWidth());
}

void ShapeContext::marker(svgpp::marker_vertex vertex, double xCoordinate, double yCoordinate,
                          double directionality, unsigned markerIndex) {
    if (markerIndex >= _markers.size()) {
        _markers.resize(markerIndex + 1);
    }

    MarkerPos& markerPosition = _markers[markerIndex];
    markerPosition.vertex = vertex;
    markerPosition.xCoordinate = xCoordinate;
    markerPosition.yCoordinate = yCoordinate;
    markerPosition.directionality = directionality;
}

} /* namespace laby::svgp */
