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

#ifndef AGG_CURVES_INCLUDED
#define AGG_CURVES_INCLUDED

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Point_2.h>
#include <CGAL/number_utils.h>
#include <array>
#include <cstddef>
#include <vector>

namespace agg {
using Kernel = CGAL::Exact_predicates_exact_constructions_kernel;
using Point = CGAL::Point_2<Kernel>;
// See Implementation agg_curves.cpp

constexpr double kQuadraticDivisor = 2.0;
constexpr double kHermiteDivisor = 3.0;
constexpr double kCatmullRomDivisor = 6.0;
constexpr double kCatmullRomDoubleWeight = 2.0;
constexpr double kCatmullRomQuadrupleWeight = 4.0;
constexpr double kCurvePiValue = 3.14159265358979323846;
constexpr std::size_t kCurve4CoordinateCount = 8;
constexpr std::size_t kCurve4StartXIndex = 0;
constexpr std::size_t kCurve4StartYIndex = 1;
constexpr std::size_t kCurve4Control1XIndex = 2;
constexpr std::size_t kCurve4Control1YIndex = 3;
constexpr std::size_t kCurve4Control2XIndex = 4;
constexpr std::size_t kCurve4Control2YIndex = 5;
constexpr std::size_t kCurve4EndXIndex = 6;
constexpr std::size_t kCurve4EndYIndex = 7;

//--------------------------------------------curve_approximation_method_e

enum path_commands_e : unsigned char {
    path_cmd_stop = 0,        //----path_cmd_stop
    path_cmd_move_to = 1,     //----path_cmd_move_to
    path_cmd_line_to = 2,     //----path_cmd_line_to
    path_cmd_curve3 = 3,      //----path_cmd_curve3
    path_cmd_curve4 = 4,      //----path_cmd_curve4
    path_cmd_curveN = 5,      //----path_cmd_curveN
    path_cmd_catrom = 6,      //----path_cmd_catrom
    path_cmd_ubspline = 7,    //----path_cmd_ubspline
    path_cmd_end_poly = 0x0F, //----path_cmd_end_poly
    path_cmd_mask = 0x0F      //----path_cmd_mask
};

//-------------------------------------------------------------curve3_div
class Curve3 {
  public:
    Curve3() = default;

    Curve3(double startX, double startY, double controlX, double controlY, double endX,
           double endY) {
        init(startX, startY, controlX, controlY, endX, endY);
    }

    static auto calcSqDistance(const Point& firstPoint, const Point& secondPoint) -> double {
        const double deltaX = CGAL::to_double(secondPoint.x() - firstPoint.x());
        const double deltaY = CGAL::to_double(secondPoint.y() - firstPoint.y());
        return deltaX * deltaX + deltaY * deltaY;
    }

    void reset() {
        _points.clear();
    }
    void init(double startX, double startY, double controlX, double controlY, double endX,
              double endY);

    void setApproximationScale(double approximationScale) {
        _approximationScale = approximationScale;
    }
    [[nodiscard]] auto approximationScale() const -> double {
        return _approximationScale;
    }

    void setAngleTolerance(double angleTolerance) {
        _angleTolerance = angleTolerance;
    }
    [[nodiscard]] auto angleTolerance() const -> double {
        return _angleTolerance;
    }

    [[nodiscard]] auto getPoints() const -> const std::vector<Point>& {
        return _points;
    }

  private:
    static constexpr double kDefaultApproximationScale = 1.0;

    void bezier(double startX, double startY, double controlX, double controlY, double endX,
                double endY);
    void recursiveBezier(double startX, double startY, double controlX, double controlY,
                         double endX, double endY, unsigned level);

    double _approximationScale = kDefaultApproximationScale;
    double _distanceToleranceSquare = 0.0;
    double _angleTolerance = 0.0;

    std::vector<Point> _points;
};

//-------------------------------------------------------------curve4_points
class Curve4Points {
  public:
    Curve4Points() = default;

    explicit Curve4Points(const std::array<double, kCurve4CoordinateCount>& coordinates)
        : _coordinates(coordinates) {}

    [[nodiscard]] auto startX() const -> double { return _coordinates.at(kCurve4StartXIndex); }
    [[nodiscard]] auto startY() const -> double { return _coordinates.at(kCurve4StartYIndex); }
    [[nodiscard]] auto control1X() const -> double {
        return _coordinates.at(kCurve4Control1XIndex);
    }
    [[nodiscard]] auto control1Y() const -> double {
        return _coordinates.at(kCurve4Control1YIndex);
    }
    [[nodiscard]] auto control2X() const -> double {
        return _coordinates.at(kCurve4Control2XIndex);
    }
    [[nodiscard]] auto control2Y() const -> double {
        return _coordinates.at(kCurve4Control2YIndex);
    }
    [[nodiscard]] auto endX() const -> double { return _coordinates.at(kCurve4EndXIndex); }
    [[nodiscard]] auto endY() const -> double { return _coordinates.at(kCurve4EndYIndex); }

  private:
    std::array<double, kCurve4CoordinateCount> _coordinates{0.0, 0.0, 0.0, 0.0,
                                                             0.0, 0.0, 0.0, 0.0};
};

//-------------------------------------------------------catrom_to_bezier
inline auto catromToBezier(double startX, double startY, double control1X, double control1Y,
                           double control2X, double control2Y, double endX,
                           double endY) -> Curve4Points {
    // Trans. matrix Catmull-Rom to Bezier
    //
    //  0       1       0       0
    //  -1/6    1       1/6     0
    //  0       1/6     1       -1/6
    //  0       0       1       0
    //
    return Curve4Points{{control1X,
                         control1Y,
                         (-startX + kCatmullRomDivisor * control1X + control2X) /
                             kCatmullRomDivisor,
                         (-startY + kCatmullRomDivisor * control1Y + control2Y) /
                             kCatmullRomDivisor,
                         (control1X + kCatmullRomDivisor * control2X - endX) /
                             kCatmullRomDivisor,
                         (control1Y + kCatmullRomDivisor * control2Y - endY) /
                             kCatmullRomDivisor,
                         control2X,
                         control2Y}};
}

//-----------------------------------------------------------------------
inline auto catromToBezier(const Curve4Points& controlPoints) -> Curve4Points {
    return catromToBezier(controlPoints.startX(), controlPoints.startY(),
                          controlPoints.control1X(), controlPoints.control1Y(),
                          controlPoints.control2X(), controlPoints.control2Y(),
                          controlPoints.endX(), controlPoints.endY());
}

//-----------------------------------------------------ubspline_to_bezier
inline auto ubsplineToBezier(double startX, double startY, double control1X, double control1Y,
                             double control2X, double control2Y, double endX,
                             double endY) -> Curve4Points {
    // Trans. matrix Uniform BSpline to Bezier
    //
    //  1/6     4/6     1/6     0
    //  0       4/6     2/6     0
    //  0       2/6     4/6     0
    //  0       1/6     4/6     1/6
    //
    return Curve4Points{{(startX + kCatmullRomQuadrupleWeight * control1X + control2X) /
                             kCatmullRomDivisor,
                         (startY + kCatmullRomQuadrupleWeight * control1Y + control2Y) /
                             kCatmullRomDivisor,
                         (kCatmullRomQuadrupleWeight * control1X +
                          kCatmullRomDoubleWeight * control2X) /
                             kCatmullRomDivisor,
                         (kCatmullRomQuadrupleWeight * control1Y +
                          kCatmullRomDoubleWeight * control2Y) /
                             kCatmullRomDivisor,
                         (kCatmullRomDoubleWeight * control1X +
                          kCatmullRomQuadrupleWeight * control2X) /
                             kCatmullRomDivisor,
                         (kCatmullRomDoubleWeight * control1Y +
                          kCatmullRomQuadrupleWeight * control2Y) /
                             kCatmullRomDivisor,
                         (control1X + kCatmullRomQuadrupleWeight * control2X + endX) /
                             kCatmullRomDivisor,
                         (control1Y + kCatmullRomQuadrupleWeight * control2Y + endY) /
                             kCatmullRomDivisor}};
}

//-----------------------------------------------------------------------
inline auto ubsplineToBezier(const Curve4Points& controlPoints) -> Curve4Points {
    return ubsplineToBezier(controlPoints.startX(), controlPoints.startY(),
                            controlPoints.control1X(), controlPoints.control1Y(),
                            controlPoints.control2X(), controlPoints.control2Y(),
                            controlPoints.endX(), controlPoints.endY());
}

//------------------------------------------------------hermite_to_bezier
inline auto hermiteToBezier(double startX, double startY, double control1X, double control1Y,
                            double control2X, double control2Y, double endX,
                            double endY) -> Curve4Points {
    // Trans. matrix Hermite to Bezier
    //
    //  1       0       0       0
    //  1       0       1/3     0
    //  0       1       0       -1/3
    //  0       1       0       0
    //
    return Curve4Points{{startX,
                         startY,
                         (kHermiteDivisor * startX + control2X) / kHermiteDivisor,
                         (kHermiteDivisor * startY + control2Y) / kHermiteDivisor,
                         (kHermiteDivisor * control1X - endX) / kHermiteDivisor,
                         (kHermiteDivisor * control1Y - endY) / kHermiteDivisor,
                         control1X,
                         control1Y}};
}

//-----------------------------------------------------------------------
inline auto hermiteToBezier(const Curve4Points& controlPoints) -> Curve4Points {
    return hermiteToBezier(controlPoints.startX(), controlPoints.startY(),
                           controlPoints.control1X(), controlPoints.control1Y(),
                           controlPoints.control2X(), controlPoints.control2Y(),
                           controlPoints.endX(), controlPoints.endY());
}

//-------------------------------------------------------------curve4_div
class Curve4 {
  public:
    Curve4() = default;

    Curve4(double startX, double startY, double control1X, double control1Y, double control2X,
           double control2Y, double endX, double endY) {
        init(startX, startY, control1X, control1Y, control2X, control2Y, endX, endY);
    }

    explicit Curve4(const Curve4Points& controlPoints) {
        init(controlPoints.startX(), controlPoints.startY(), controlPoints.control1X(),
             controlPoints.control1Y(), controlPoints.control2X(), controlPoints.control2Y(),
             controlPoints.endX(), controlPoints.endY());
    }

    static auto calcSqDistance(const Point& firstPoint, const Point& secondPoint) -> double {
        const double deltaX = CGAL::to_double(secondPoint.x() - firstPoint.x());
        const double deltaY = CGAL::to_double(secondPoint.y() - firstPoint.y());
        return deltaX * deltaX + deltaY * deltaY;
    }

    void reset() {
        _points.clear();
    }

    void init(double startX, double startY, double control1X, double control1Y, double control2X,
              double control2Y, double endX, double endY);

    void init(const Curve4Points& controlPoints) {
        init(controlPoints.startX(), controlPoints.startY(), controlPoints.control1X(),
             controlPoints.control1Y(), controlPoints.control2X(), controlPoints.control2Y(),
             controlPoints.endX(), controlPoints.endY());
    }

    void setApproximationScale(double approximationScale) {
        _approximationScale = approximationScale;
    }
    [[nodiscard]] auto approximationScale() const -> double {
        return _approximationScale;
    }

    void setAngleTolerance(double angleTolerance) {
        _angleTolerance = angleTolerance;
    }
    [[nodiscard]] auto angleTolerance() const -> double {
        return _angleTolerance;
    }

    void setCuspLimit(double cuspLimit) {
        _cuspLimit = (cuspLimit == 0.0) ? 0.0 : kCurvePiValue - cuspLimit;
    }

    [[nodiscard]] auto cuspLimit() const -> double {
        return (_cuspLimit == 0.0) ? 0.0 : kCurvePiValue - _cuspLimit;
    }

    [[nodiscard]] auto getPoints() const -> const std::vector<Point>& {
        return _points;
    }

  private:
    static constexpr double kDefaultApproximationScale = 1.0;

    void bezier(double startX, double startY, double control1X, double control1Y, double control2X,
                double control2Y, double endX, double endY);

    void recursiveBezier(double startX, double startY, double control1X, double control1Y,
                         double control2X, double control2Y, double endX, double endY,
                         unsigned level);

    double _approximationScale = kDefaultApproximationScale;
    double _distanceToleranceSquare = 0.0;
    double _angleTolerance = 0.0;
    double _cuspLimit = 0.0;
    std::vector<Point> _points;
};

//-----------------------------------------------------------------curve4

} // namespace agg

#endif
