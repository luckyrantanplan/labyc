/*
 * agg_arc.cpp
 *
 *  Created on: Jul 4, 2018
 *      Author: florian
 */
#include "agg_arc.h"
#include <algorithm> // std::reverse
#include <cmath>

//----------------------------------------------------------------------------
// Anti-Grain Geometry - Version 2.4
// Copyright (C) 2002-2005 Maxim Shemanarev (http://www.antigrain.com)
//
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all copies.
// This software is provided "as is" without express or implied
// warranty, and with no claim as to its suitability for any purpose.
//
//----------------------------------------------------------------------------
// Contact: mcseem@antigrain.com
//          mcseemagg@yahoo.com
//          http://www.antigrain.com
//----------------------------------------------------------------------------
//
// Arc vertex generator
//
//----------------------------------------------------------------------------

namespace agg {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kAngleStepMultiplier = 2.0;
constexpr double kFullTurn = 2.0 * kPi;
constexpr double kQuarterTurnSegments = 4.0;
constexpr double kApproximationOffset = 0.125;
} // namespace

//------------------------------------------------------------------------
Arc::Arc(const Point& centerPoint, double radius, const Point& startPoint, const Point& endPoint,
         bool isCounterClockwise) {

    const CGAL::Vector_2<Kernel> startVector = startPoint - centerPoint;
    double startAngle =
        std::atan2(CGAL::to_double(startVector.y()), CGAL::to_double(startVector.x()));

    const CGAL::Vector_2<Kernel> endVector = endPoint - centerPoint;
    double endAngle = std::atan2(CGAL::to_double(endVector.y()), CGAL::to_double(endVector.x()));

    if (!isCounterClockwise) {
        std::swap(startAngle, endAngle);
    }

    init(ArcParameters{
        centerPoint,
        radius,
        startAngle,
        endAngle,
    });

    if (!isCounterClockwise) {
        std::reverse(_points.begin(), _points.end());
    }
    _points.front() = startPoint;
    _points.back() = endPoint;
}

//------------------------------------------------------------------------
void Arc::init(const ArcParameters& parameters) {

    normalize(parameters);

    _angle = _start;
    while (true) {

        if (_angle > _end - _deltaAngle / kQuarterTurnSegments) {
            _points.emplace_back(
                CGAL::to_double(parameters.center.x()) + std::cos(_end) * parameters.radius,
                CGAL::to_double(parameters.center.y()) + std::sin(_end) * parameters.radius);
            break;
        }

        _points.emplace_back(
            CGAL::to_double(parameters.center.x()) + std::cos(_angle) * parameters.radius,
            CGAL::to_double(parameters.center.y()) + std::sin(_angle) * parameters.radius);
        _angle += _deltaAngle;
    }
}

//------------------------------------------------------------------------
void Arc::normalize(const ArcParameters& parameters) {
    const double absoluteRadius = std::fabs(parameters.radius);
    double normalizedEndAngle = parameters.endAngle;
    _deltaAngle = std::acos(absoluteRadius / (absoluteRadius + kApproximationOffset / _scale)) *
                  kAngleStepMultiplier;

    while (normalizedEndAngle < parameters.startAngle) {
        normalizedEndAngle += kFullTurn;
    }
    // ccw only
    if (_deltaAngle >= (normalizedEndAngle - parameters.startAngle)) {
        _deltaAngle = (normalizedEndAngle - parameters.startAngle) / kQuarterTurnSegments;
    }

    _start = parameters.startAngle;
    _end = normalizedEndAngle;
}

} /* namespace agg */
