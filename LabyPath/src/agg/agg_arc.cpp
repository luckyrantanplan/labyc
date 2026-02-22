/*
 * agg_arc.cpp
 *
 *  Created on: Jul 4, 2018
 *      Author: florian
 */
#include <math.h>
#include <algorithm>    // std::reverse
#include "agg_arc.h"

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

//------------------------------------------------------------------------
Arc::Arc(const CGAL::Point_2<Kernel>& center, double r, const CGAL::Point_2<Kernel>& pStart, const CGAL::Point_2<Kernel>& pEnd, bool ccw) {

    CGAL::Vector_2<Kernel> va = pStart - center;

    double a1 = atan2(CGAL::to_double(va.y()), CGAL::to_double(va.x()));

    CGAL::Vector_2<Kernel> vb = pEnd - center;
    double a2 = atan2(CGAL::to_double(vb.y()), CGAL::to_double(vb.x()));

    if (!ccw) {
        std::swap(a1, a2);
    }

    init(CGAL::to_double(center.x()), CGAL::to_double(center.y()), r, a1, a2);

    if (!ccw) {
        std::reverse(m_points.begin(), m_points.end());
    }
    m_points.front() = pStart;
    m_points.back() = pEnd;
}

//------------------------------------------------------------------------
void Arc::init(double x, double y, double r, double a1, double a2) {

    normalize(a1, a2, r);

    m_angle = m_start;
    while (true) {

        if (m_angle > m_end - m_da / 4.) {
            m_points.emplace_back(x + cos(m_end) * r, y + sin(m_end) * r);
            break;
        }

        m_points.emplace_back(x + cos(m_angle) * r, y + sin(m_angle) * r);
        m_angle += m_da;
    }
}

//------------------------------------------------------------------------
void Arc::normalize(double a1, double a2, double r) {
    double ra = fabs(r);
    m_da = acos(ra / (ra + 0.125 / m_scale)) * 2;

    while (a2 < a1) {
        a2 += 2.0 * M_PI;
    }
    // ccw only
    if (m_da >= (a2 - a1)) {
        m_da = (a2 - a1) / 4.;
    }

    m_start = a1;
    m_end = a2;

}

} /* namespace agg */
