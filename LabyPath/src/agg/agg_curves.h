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
#include <vector>

namespace agg {
typedef CGAL::Exact_predicates_exact_constructions_kernel Kernel;
// See Implementation agg_curves.cpp

//--------------------------------------------curve_approximation_method_e

enum path_commands_e {
    path_cmd_stop = 0,        //----path_cmd_stop
    path_cmd_move_to = 1,        //----path_cmd_move_to
    path_cmd_line_to = 2,        //----path_cmd_line_to
    path_cmd_curve3 = 3,        //----path_cmd_curve3
    path_cmd_curve4 = 4,        //----path_cmd_curve4
    path_cmd_curveN = 5,        //----path_cmd_curveN
    path_cmd_catrom = 6,        //----path_cmd_catrom
    path_cmd_ubspline = 7,        //----path_cmd_ubspline
    path_cmd_end_poly = 0x0F,     //----path_cmd_end_poly
    path_cmd_mask = 0x0F      //----path_cmd_mask
};

//-------------------------------------------------------------curve3_div
class Curve3 {
public:
    Curve3() {
    }

    Curve3(double x1, double y1, double x2, double y2, double x3, double y3) {
        init(x1, y1, x2, y2, x3, y3);
    }

    double calc_sq_distance(double x1, double y1, double x2, double y2) {
        double dx = x2 - x1;
        double dy = y2 - y1;
        return dx * dx + dy * dy;
    }

    void reset() {
        m_points.clear();

    }
    void init(double x1, double y1, double x2, double y2, double x3, double y3);

    void approximation_scale(double s) {
        m_approximation_scale = s;
    }
    double approximation_scale() const {
        return m_approximation_scale;
    }

    void angle_tolerance(double a) {
        m_angle_tolerance = a;
    }
    double angle_tolerance() const {
        return m_angle_tolerance;
    }

    const std::vector<CGAL::Point_2<Kernel> >& getPoints() const {
        return m_points;
    }

private:
    void bezier(double x1, double y1, double x2, double y2, double x3, double y3);
    void recursive_bezier(double x1, double y1, double x2, double y2, double x3, double y3, unsigned level);

    double m_approximation_scale = 1.0;
    double m_distance_tolerance_square = 0;
    double m_angle_tolerance = 0.;

    std::vector<CGAL::Point_2<Kernel>> m_points;
};

//-------------------------------------------------------------curve4_points
struct curve4_points {
    double cp[8];
    curve4_points() {
    }
    curve4_points(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4) {
        cp[0] = x1;
        cp[1] = y1;
        cp[2] = x2;
        cp[3] = y2;
        cp[4] = x3;
        cp[5] = y3;
        cp[6] = x4;
        cp[7] = y4;
    }
    void init(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4) {
        cp[0] = x1;
        cp[1] = y1;
        cp[2] = x2;
        cp[3] = y2;
        cp[4] = x3;
        cp[5] = y3;
        cp[6] = x4;
        cp[7] = y4;
    }
    double operator [](unsigned i) const {
        return cp[i];
    }
    double& operator [](unsigned i) {
        return cp[i];
    }
};

//-------------------------------------------------------catrom_to_bezier
inline curve4_points catrom_to_bezier(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4) {
    // Trans. matrix Catmull-Rom to Bezier
    //
    //  0       1       0       0
    //  -1/6    1       1/6     0
    //  0       1/6     1       -1/6
    //  0       0       1       0
    //
    return curve4_points(x2, y2, (-x1 + 6 * x2 + x3) / 6, (-y1 + 6 * y2 + y3) / 6, (x2 + 6 * x3 - x4) / 6, (y2 + 6 * y3 - y4) / 6, x3, y3);
}

//-----------------------------------------------------------------------
inline curve4_points catrom_to_bezier(const curve4_points& cp) {
    return catrom_to_bezier(cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
}

//-----------------------------------------------------ubspline_to_bezier
inline curve4_points ubspline_to_bezier(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4) {
    // Trans. matrix Uniform BSpline to Bezier
    //
    //  1/6     4/6     1/6     0
    //  0       4/6     2/6     0
    //  0       2/6     4/6     0
    //  0       1/6     4/6     1/6
    //
    return curve4_points((x1 + 4 * x2 + x3) / 6, (y1 + 4 * y2 + y3) / 6, (4 * x2 + 2 * x3) / 6, (4 * y2 + 2 * y3) / 6, (2 * x2 + 4 * x3) / 6, (2 * y2 + 4 * y3) / 6, (x2 + 4 * x3 + x4) / 6,
            (y2 + 4 * y3 + y4) / 6);
}

//-----------------------------------------------------------------------
inline curve4_points ubspline_to_bezier(const curve4_points& cp) {
    return ubspline_to_bezier(cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
}

//------------------------------------------------------hermite_to_bezier
inline curve4_points hermite_to_bezier(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4) {
    // Trans. matrix Hermite to Bezier
    //
    //  1       0       0       0
    //  1       0       1/3     0
    //  0       1       0       -1/3
    //  0       1       0       0
    //
    return curve4_points(x1, y1, (3 * x1 + x3) / 3, (3 * y1 + y3) / 3, (3 * x2 - x4) / 3, (3 * y2 - y4) / 3, x2, y2);
}

//-----------------------------------------------------------------------
inline curve4_points hermite_to_bezier(const curve4_points& cp) {
    return hermite_to_bezier(cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
}

//-------------------------------------------------------------curve4_div
class Curve4 {
public:
    Curve4() {
    }

    Curve4(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4) {
        init(x1, y1, x2, y2, x3, y3, x4, y4);
    }

    explicit Curve4(const curve4_points& cp) {
        init(cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
    }

    double calc_sq_distance(double x1, double y1, double x2, double y2) {
        double dx = x2 - x1;
        double dy = y2 - y1;
        return dx * dx + dy * dy;
    }

    void reset() {
        m_points.clear();
    }

    void init(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4);

    void init(const curve4_points& cp) {
        init(cp[0], cp[1], cp[2], cp[3], cp[4], cp[5], cp[6], cp[7]);
    }

    void approximation_scale(double s) {
        m_approximation_scale = s;
    }
    double approximation_scale() const {
        return m_approximation_scale;
    }

    void angle_tolerance(double a) {
        m_angle_tolerance = a;
    }
    double angle_tolerance() const {
        return m_angle_tolerance;
    }

    void cusp_limit(double v) {
        m_cusp_limit = (v == 0.0) ? 0.0 : M_PI - v;
    }

    double cusp_limit() const {
        return (m_cusp_limit == 0.0) ? 0.0 : M_PI - m_cusp_limit;
    }

    const std::vector<CGAL::Point_2<Kernel> >& getPoints() const {
        return m_points;
    }

private:
    void bezier(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4);

    void recursive_bezier(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4, unsigned level);

    double m_approximation_scale = 1;
    double m_distance_tolerance_square = 0;
    double m_angle_tolerance = 0;
    double m_cusp_limit = 0;
    std::vector<CGAL::Point_2<Kernel>> m_points;
};

//-----------------------------------------------------------------curve4

}

#endif
