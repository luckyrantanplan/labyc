/*
 * ShapeContext2.cpp
 *
 *  Created on: Jun 18, 2018
 *      Author: florian
 */

#include "ShapeContext.h"

#include <boost/variant/get.hpp>
#include <CGAL/number_utils.h>
#include <CGAL/Point_2.h>
#include <svgpp/definitions.hpp>
#include <iostream>

#include "../agg/agg_curves.h"
#include "../GeomData.h"

namespace laby {
namespace svgp {

ShapeContext::ShapeContext(BaseContext & parent) :
        BaseContext(parent) {

    _vectRibbonRef.emplace_back();
    _currentRibbon = &_vectRibbonRef.back();

}

void ShapeContext::path_move_to(double x, double y, svgpp::tag::coordinate::absolute) {

    if (_currentRibbon->lines().empty() or !_currentRibbon->lines().back().points.empty()) {
        _currentRibbon->lines().emplace_back();
    }

    _currentRibbon->lines().back().points.emplace_back(Point_2(x, y));
}

void ShapeContext::path_line_to(double x, double y, svgpp::tag::coordinate::absolute) {
    _currentRibbon->lines().back().points.emplace_back(Point_2(x, y));
}

void ShapeContext::path_cubic_bezier_to(double x1, double y1, double x2, double y2, double x, double y, svgpp::tag::coordinate::absolute) {

    {
        Point_2& ref = _currentRibbon->lines().back().points.back();
        agg::Curve4 curve(CGAL::to_double(ref.x()), CGAL::to_double(ref.y()), x1, y1, x2, y2, x, y);

        const auto& vect = curve.getPoints();
        auto& ribbonLine = _currentRibbon->lines().back().points;
        //remove last item
        ribbonLine.insert(ribbonLine.end(), ++vect.begin(), vect.end());
    }

}

void ShapeContext::path_quadratic_bezier_to(double x1, double y1, double x, double y, svgpp::tag::coordinate::absolute) {

    Point_2& ref = _currentRibbon->lines().back().points.back();
    agg::Curve3 curve(CGAL::to_double(ref.x()), CGAL::to_double(ref.y()), x1, y1, x, y);

    const auto& vect = curve.getPoints();
    auto& ribbonLine = _currentRibbon->lines().back().points;
//remove last item
    ribbonLine.insert(ribbonLine.end(), ++vect.begin(), vect.end());

}

void ShapeContext::path_elliptical_arc_to(double rx, double ry, double x_axis_rotation, bool large_arc_flag, bool sweep_flag, double x, double y, svgpp::tag::coordinate::absolute) {
    std::cout << "path_elliptical_arc_to " << x << " " << y << std::endl;
}

void ShapeContext::path_close_subpath() {
    _currentRibbon->lines().back().closed = true;
    std::vector<Point_2>& points = _currentRibbon->lines().back().points;
    if (points.back() != points.front()) {
        points.emplace_back(points.front());
    }
    _currentRibbon->lines().emplace_back();
}

color_t ShapeContext::getColor(const Paint& paint) {
    EffectivePaint epaint = style().getEffectivePaint(paint);
    if (boost::get<color_t>(&epaint)) {
        return boost::get<color_t>(epaint);
    }
    return 0;
}

void ShapeContext::path_exit() {
    _currentRibbon->set_fill_color(getColor(style().fill_paint_));
    _currentRibbon->setStrokeColor(getColor(style().stroke_paint_));
    _currentRibbon->setStrokeWidth(style().stroke_width_);

}

void ShapeContext::marker(svgpp::marker_vertex v, double x, double y, double directionality, unsigned marker_index) {
    if (marker_index >= markers_.size())
        markers_.resize(marker_index + 1);

    MarkerPos& m = markers_[marker_index];
    m.v = v;
    m.x = x;
    m.y = y;
    m.directionality = directionality;
}

} /* namespace svgp */
} /* namespace laby */
