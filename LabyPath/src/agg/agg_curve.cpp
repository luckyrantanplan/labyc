//----------------------------------------------------------------------------
// Anti-Grain Geometry (AGG) - Version 2.5
// A high quality rendering engine for C++
// Copyright (C) 2002-2006 Maxim Shemanarev
// Contact: mcseem@antigrain.com
//          mcseemagg@yahoo.com
//          http://antigrain.com
//
// AGG is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// AGG is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with AGG; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA 02110-1301, USA.
//----------------------------------------------------------------------------

#include "agg_curves.h"
#include <cmath>

namespace agg {

//------------------------------------------------------------------------
constexpr double kCurveDistanceEpsilon = 1e-30;
constexpr double kCurveCollinearityEpsilon = 1e-30;
constexpr double kCurveAngleToleranceEpsilon = 0.01;
constexpr double kHalf = 0.5;
constexpr unsigned kCurveRecursionLimit = 32U;

namespace {

struct Curve3Segment {
    double startX;
    double startY;
    double controlX;
    double controlY;
    double endX;
    double endY;
};

struct Curve4Segment {
    double startX;
    double startY;
    double control1X;
    double control1Y;
    double control2X;
    double control2Y;
    double endX;
    double endY;
};

struct Curve3ApproximationConfig {
    double distanceToleranceSquare;
    double angleTolerance;
};

struct Curve4ApproximationConfig {
    double distanceToleranceSquare;
    double angleTolerance;
    double cuspLimit;
};

struct Curve4Midpoints {
    double x12;
    double y12;
    double x23;
    double y23;
    double x34;
    double y34;
    double x123;
    double y123;
    double x234;
    double y234;
    double x1234;
    double y1234;
};

auto computeCurve4Midpoints(const Curve4Segment& segment) -> Curve4Midpoints {
    const double x12 = (segment.startX + segment.control1X) / kQuadraticDivisor;
    const double y12 = (segment.startY + segment.control1Y) / kQuadraticDivisor;
    const double x23 = (segment.control1X + segment.control2X) / kQuadraticDivisor;
    const double y23 = (segment.control1Y + segment.control2Y) / kQuadraticDivisor;
    const double x34 = (segment.control2X + segment.endX) / kQuadraticDivisor;
    const double y34 = (segment.control2Y + segment.endY) / kQuadraticDivisor;
    const double x123 = (x12 + x23) / kQuadraticDivisor;
    const double y123 = (y12 + y23) / kQuadraticDivisor;
    const double x234 = (x23 + x34) / kQuadraticDivisor;
    const double y234 = (y23 + y34) / kQuadraticDivisor;
    return {x12,
            y12,
            x23,
            y23,
            x34,
            y34,
            x123,
            y123,
            x234,
            y234,
            (x123 + x234) / kQuadraticDivisor,
            (y123 + y234) / kQuadraticDivisor};
}

auto handleCurve3RegularSegment(const Curve3Segment& segment, double midpointX, double midpointY,
                                const Curve3ApproximationConfig& config,
                                std::vector<Point>& points) -> bool {
    const double deltaX = segment.endX - segment.startX;
    const double deltaY = segment.endY - segment.startY;
    const double distanceToChord = std::fabs(
        ((segment.controlX - segment.endX) * deltaY - (segment.controlY - segment.endY) * deltaX));
    if (distanceToChord * distanceToChord >
        config.distanceToleranceSquare * (deltaX * deltaX + deltaY * deltaY)) {
        return false;
    }
    if (config.angleTolerance < kCurveAngleToleranceEpsilon) {
        points.emplace_back(midpointX, midpointY);
        return true;
    }

    double angleDelta =
        std::fabs(std::atan2(segment.endY - segment.controlY, segment.endX - segment.controlX) -
                  std::atan2(segment.controlY - segment.startY, segment.controlX - segment.startX));
    if (angleDelta >= M_PI) {
        angleDelta = 2 * M_PI - angleDelta;
    }
    if (angleDelta < config.angleTolerance) {
        points.emplace_back(midpointX, midpointY);
        return true;
    }
    return false;
}

auto handleCurve3CollinearSegment(const Curve3Segment& segment,
                                  const Curve3ApproximationConfig& config,
                                  std::vector<Point>& points) -> bool {
    const double deltaX = segment.endX - segment.startX;
    const double deltaY = segment.endY - segment.startY;
    double projectionLength = deltaX * deltaX + deltaY * deltaY;
    if (projectionLength == 0) {
        projectionLength = Curve3::calcSqDistance(Point(segment.startX, segment.startY),
                                                  Point(segment.controlX, segment.controlY));
    } else {
        projectionLength = ((segment.controlX - segment.startX) * deltaX +
                            (segment.controlY - segment.startY) * deltaY) /
                           projectionLength;
        if (projectionLength > 0 && projectionLength < 1) {
            return true;
        }
        if (projectionLength <= 0) {
            projectionLength = Curve3::calcSqDistance(Point(segment.controlX, segment.controlY),
                                                      Point(segment.startX, segment.startY));
        } else if (projectionLength >= 1) {
            projectionLength = Curve3::calcSqDistance(Point(segment.controlX, segment.controlY),
                                                      Point(segment.endX, segment.endY));
        } else {
            projectionLength =
                Curve3::calcSqDistance(Point(segment.controlX, segment.controlY),
                                       Point(segment.startX + projectionLength * deltaX,
                                             segment.startY + projectionLength * deltaY));
        }
    }
    if (projectionLength < config.distanceToleranceSquare) {
        points.emplace_back(segment.controlX, segment.controlY);
        return true;
    }
    return false;
}

auto handleCurve4CollinearCase(const Curve4Segment& segment,
                               const Curve4ApproximationConfig& config,
                               std::vector<Point>& points) -> bool {
    const double deltaX = segment.endX - segment.startX;
    const double deltaY = segment.endY - segment.startY;
    double projectionFactor = deltaX * deltaX + deltaY * deltaY;
    double distanceControl1 = 0.0;
    double distanceControl2 = 0.0;
    if (projectionFactor == 0) {
        distanceControl1 = Curve4::calcSqDistance(Point(segment.startX, segment.startY),
                                                  Point(segment.control1X, segment.control1Y));
        distanceControl2 = Curve4::calcSqDistance(Point(segment.endX, segment.endY),
                                                  Point(segment.control2X, segment.control2Y));
    } else {
        projectionFactor = 1 / projectionFactor;
        const double control1ProjectionX = segment.control1X - segment.startX;
        const double control1ProjectionY = segment.control1Y - segment.startY;
        const double control2ProjectionX = segment.control2X - segment.startX;
        const double control2ProjectionY = segment.control2Y - segment.startY;
        distanceControl1 =
            projectionFactor * (control1ProjectionX * deltaX + control1ProjectionY * deltaY);
        distanceControl2 =
            projectionFactor * (control2ProjectionX * deltaX + control2ProjectionY * deltaY);
        if (distanceControl1 > 0 && distanceControl1 < 1 && distanceControl2 > 0 &&
            distanceControl2 < 1) {
            return true;
        }
        if (distanceControl1 <= 0) {
            distanceControl1 = Curve4::calcSqDistance(Point(segment.control1X, segment.control1Y),
                                                      Point(segment.startX, segment.startY));
        } else if (distanceControl1 >= 1) {
            distanceControl1 = Curve4::calcSqDistance(Point(segment.control1X, segment.control1Y),
                                                      Point(segment.endX, segment.endY));
        } else {
            distanceControl1 =
                Curve4::calcSqDistance(Point(segment.control1X, segment.control1Y),
                                       Point(segment.startX + distanceControl1 * deltaX,
                                             segment.startY + distanceControl1 * deltaY));
        }

        if (distanceControl2 <= 0) {
            distanceControl2 = Curve4::calcSqDistance(Point(segment.control2X, segment.control2Y),
                                                      Point(segment.startX, segment.startY));
        } else if (distanceControl2 >= 1) {
            distanceControl2 = Curve4::calcSqDistance(Point(segment.control2X, segment.control2Y),
                                                      Point(segment.endX, segment.endY));
        } else {
            distanceControl2 =
                Curve4::calcSqDistance(Point(segment.control2X, segment.control2Y),
                                       Point(segment.startX + distanceControl2 * deltaX,
                                             segment.startY + distanceControl2 * deltaY));
        }
    }

    if (distanceControl1 > distanceControl2) {
        if (distanceControl1 < config.distanceToleranceSquare) {
            points.emplace_back(segment.control1X, segment.control1Y);
            return true;
        }
        return false;
    }
    if (distanceControl2 < config.distanceToleranceSquare) {
        points.emplace_back(segment.control2X, segment.control2Y);
        return true;
    }
    return false;
}

auto handleCurve4SecondControlCase(const Curve4Segment& segment, const Curve4Midpoints& midpoints,
                                   const Curve4ApproximationConfig& config,
                                   std::vector<Point>& points) -> bool {
    const double deltaX = segment.endX - segment.startX;
    const double deltaY = segment.endY - segment.startY;
    const double distanceControl2 = std::fabs(((segment.control2X - segment.endX) * deltaY -
                                               (segment.control2Y - segment.endY) * deltaX));
    if (distanceControl2 * distanceControl2 >
        config.distanceToleranceSquare * (deltaX * deltaX + deltaY * deltaY)) {
        return false;
    }
    if (config.angleTolerance < kCurveAngleToleranceEpsilon) {
        points.emplace_back(midpoints.x23, midpoints.y23);
        return true;
    }
    double angleDelta = std::fabs(
        std::atan2(segment.endY - segment.control2Y, segment.endX - segment.control2X) -
        std::atan2(segment.control2Y - segment.control1Y, segment.control2X - segment.control1X));
    if (angleDelta >= M_PI) {
        angleDelta = 2 * M_PI - angleDelta;
    }
    if (angleDelta < config.angleTolerance) {
        points.emplace_back(segment.control1X, segment.control1Y);
        points.emplace_back(segment.control2X, segment.control2Y);
        return true;
    }
    if (config.cuspLimit != 0.0 && angleDelta > config.cuspLimit) {
        points.emplace_back(segment.control2X, segment.control2Y);
        return true;
    }
    return false;
}

auto handleCurve4FirstControlCase(const Curve4Segment& segment, const Curve4Midpoints& midpoints,
                                  const Curve4ApproximationConfig& config,
                                  std::vector<Point>& points) -> bool {
    const double deltaX = segment.endX - segment.startX;
    const double deltaY = segment.endY - segment.startY;
    const double distanceControl1 = std::fabs(((segment.control1X - segment.endX) * deltaY -
                                               (segment.control1Y - segment.endY) * deltaX));
    if (distanceControl1 * distanceControl1 >
        config.distanceToleranceSquare * (deltaX * deltaX + deltaY * deltaY)) {
        return false;
    }
    if (config.angleTolerance < kCurveAngleToleranceEpsilon) {
        points.emplace_back(midpoints.x23, midpoints.y23);
        return true;
    }
    double angleDelta = std::fabs(
        std::atan2(segment.control2Y - segment.control1Y, segment.control2X - segment.control1X) -
        std::atan2(segment.control1Y - segment.startY, segment.control1X - segment.startX));
    if (angleDelta >= M_PI) {
        angleDelta = 2 * M_PI - angleDelta;
    }
    if (angleDelta < config.angleTolerance) {
        points.emplace_back(segment.control1X, segment.control1Y);
        points.emplace_back(segment.control2X, segment.control2Y);
        return true;
    }
    if (config.cuspLimit != 0.0 && angleDelta > config.cuspLimit) {
        points.emplace_back(segment.control1X, segment.control1Y);
        return true;
    }
    return false;
}

auto handleCurve4RegularCase(const Curve4Segment& segment, const Curve4Midpoints& midpoints,
                             const Curve4ApproximationConfig& config,
                             std::vector<Point>& points) -> bool {
    const double deltaX = segment.endX - segment.startX;
    const double deltaY = segment.endY - segment.startY;
    const double distanceControl1 = std::fabs(((segment.control1X - segment.endX) * deltaY -
                                               (segment.control1Y - segment.endY) * deltaX));
    const double distanceControl2 = std::fabs(((segment.control2X - segment.endX) * deltaY -
                                               (segment.control2Y - segment.endY) * deltaX));
    if ((distanceControl1 + distanceControl2) * (distanceControl1 + distanceControl2) >
        config.distanceToleranceSquare * (deltaX * deltaX + deltaY * deltaY)) {
        return false;
    }
    if (config.angleTolerance < kCurveAngleToleranceEpsilon) {
        points.emplace_back(midpoints.x23, midpoints.y23);
        return true;
    }

    const double inflectionAngle =
        std::atan2(segment.control2Y - segment.control1Y, segment.control2X - segment.control1X);
    double firstAngleDelta =
        std::fabs(inflectionAngle - std::atan2(segment.control1Y - segment.startY,
                                               segment.control1X - segment.startX));
    double secondAngleDelta =
        std::fabs(std::atan2(segment.endY - segment.control2Y, segment.endX - segment.control2X) -
                  inflectionAngle);
    if (firstAngleDelta >= M_PI) {
        firstAngleDelta = 2 * M_PI - firstAngleDelta;
    }
    if (secondAngleDelta >= M_PI) {
        secondAngleDelta = 2 * M_PI - secondAngleDelta;
    }

    if (firstAngleDelta + secondAngleDelta < config.angleTolerance) {
        points.emplace_back(midpoints.x23, midpoints.y23);
    }
    if (config.cuspLimit != 0.0 && firstAngleDelta > config.cuspLimit) {
        points.emplace_back(segment.control1X, segment.control1Y);
        return true;
    }
    if (config.cuspLimit != 0.0 && secondAngleDelta > config.cuspLimit) {
        points.emplace_back(segment.control2X, segment.control2Y);
        return true;
    }
    return false;
}

} // namespace

//------------------------------------------------------------------------
void Curve3::init(double startX, double startY, double controlX, double controlY, double endX,
                  double endY) {
    _points.clear();
    _distanceToleranceSquare = kHalf / _approximationScale;
    _distanceToleranceSquare *= _distanceToleranceSquare;
    bezier(startX, startY, controlX, controlY, endX, endY);
}

//------------------------------------------------------------------------
void Curve3::recursiveBezier(double startX, double startY, double controlX, double controlY,
                             double endX, double endY,
                             unsigned level) { 
    if (level > kCurveRecursionLimit) {
        return;
    }

    const Curve3Segment segment{startX, startY, controlX, controlY, endX, endY};
    const Curve3ApproximationConfig config{_distanceToleranceSquare, _angleTolerance};

    // Calculate all the mid-points of the line segments
    //----------------------
    double x12 = (startX + controlX) / kQuadraticDivisor;
    double y12 = (startY + controlY) / kQuadraticDivisor;
    double x23 = (controlX + endX) / kQuadraticDivisor;
    double y23 = (controlY + endY) / kQuadraticDivisor;
    double x123 = (x12 + x23) / kQuadraticDivisor;
    double y123 = (y12 + y23) / kQuadraticDivisor;

    const double distanceToChord =
        std::fabs(((controlX - endX) * (endY - startY) - (controlY - endY) * (endX - startX)));
    if (distanceToChord > kCurveCollinearityEpsilon) {
        if (handleCurve3RegularSegment(segment, x123, y123, config, _points)) {
            return;
        }
    } else if (handleCurve3CollinearSegment(segment, config, _points)) {
        return;
    }

    // Continue subdivision
    //----------------------
    recursiveBezier(startX, startY, x12, y12, x123, y123, level + 1);
    recursiveBezier(x123, y123, x23, y23, endX, endY, level + 1);
}

//------------------------------------------------------------------------
void Curve3::bezier(double startX, double startY, double controlX, double controlY, double endX,
                    double endY) {
    _points.emplace_back(startX, startY);
    recursiveBezier(startX, startY, controlX, controlY, endX, endY, 0);
    _points.emplace_back(endX, endY);
}

//------------------------------------------------------------------------
void Curve4::init(double startX, double startY, double control1X, double control1Y,
                  double control2X, double control2Y, double endX, double endY) {
    _points.clear();
    _distanceToleranceSquare = kHalf / _approximationScale;
    _distanceToleranceSquare *= _distanceToleranceSquare;
    bezier(startX, startY, control1X, control1Y, control2X, control2Y, endX, endY);
}

//------------------------------------------------------------------------
void Curve4::recursiveBezier(double startX, double startY, double control1X, double control1Y,
                             double control2X, double control2Y, double endX, double endY,
                             unsigned level) { 
    if (level > kCurveRecursionLimit) {
        return;
    }

    const Curve4Segment segment{startX,    startY,    control1X, control1Y,
                                control2X, control2Y, endX,      endY};
    const Curve4Midpoints midpoints = computeCurve4Midpoints(segment);
    const Curve4ApproximationConfig config{_distanceToleranceSquare, _angleTolerance, _cuspLimit};

    // Calculate all the mid-points of the line segments
    //----------------------
    const double x12 = midpoints.x12;
    const double y12 = midpoints.y12;
    const double x34 = midpoints.x34;
    const double y34 = midpoints.y34;
    const double x123 = midpoints.x123;
    const double y123 = midpoints.y123;
    const double x234 = midpoints.x234;
    const double y234 = midpoints.y234;
    const double x1234 = midpoints.x1234;
    const double y1234 = midpoints.y1234;

    // Try to approximate the full cubic curve by a single straight line
    //------------------
    const double deltaX = endX - startX;
    const double deltaY = endY - startY;
    const double distanceControl1 =
        std::fabs(((control1X - endX) * deltaY - (control1Y - endY) * deltaX));
    const double distanceControl2 =
        std::fabs(((control2X - endX) * deltaY - (control2Y - endY) * deltaX));
    const auto control1OffLine =
        static_cast<unsigned>(distanceControl1 > kCurveCollinearityEpsilon);
    const auto control2OffLine =
        static_cast<unsigned>(distanceControl2 > kCurveCollinearityEpsilon);

    switch ((control1OffLine << 1U) + control2OffLine) {
    case 0:
        if (handleCurve4CollinearCase(segment, config, _points)) {
            return;
        }
        break;

    case 1:
        if (handleCurve4SecondControlCase(segment, midpoints, config, _points)) {
            return;
        }
        break;

    case 2:
        if (handleCurve4FirstControlCase(segment, midpoints, config, _points)) {
            return;
        }
        break;

    case 3:
        if (handleCurve4RegularCase(segment, midpoints, config, _points)) {
            return;
        }
        break;

    default:
        break;
    }

    // Continue subdivision
    //----------------------
    recursiveBezier(startX, startY, x12, y12, x123, y123, x1234, y1234, level + 1);
    recursiveBezier(x1234, y1234, x234, y234, x34, y34, endX, endY, level + 1);
}

//------------------------------------------------------------------------
void Curve4::bezier(double startX, double startY, double control1X, double control1Y,
                    double control2X, double control2Y, double endX, double endY) {
    _points.emplace_back(startX, startY);
    recursiveBezier(startX, startY, control1X, control1Y, control2X, control2Y, endX, endY, 0);
    _points.emplace_back(endX, endY);
}

} // namespace agg
