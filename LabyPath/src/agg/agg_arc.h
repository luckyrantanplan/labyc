/*
 * agg_arc.h
 *
 *  Created on: Jul 4, 2018
 *      Author: florian
 */

#ifndef AGG_AGG_ARC_H_
#define AGG_AGG_ARC_H_

#include <math.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Point_2.h>
#include <vector>

namespace agg {

class Arc {
    typedef CGAL::Exact_predicates_exact_constructions_kernel Kernel;
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

public:

    Arc(const CGAL::Point_2<Kernel>& center, double r, const CGAL::Point_2<Kernel>& pStart, const CGAL::Point_2<Kernel>& pEnd, bool ccw = true);

    void init(double x, double y, double r, double a1, double a2);

    const std::vector<CGAL::Point_2<Kernel> >& getPoints() const {
        return m_points;
    }

private:

    std::vector<CGAL::Point_2<Kernel>> m_points;

    void normalize(double a1, double a2, double r);

    double m_angle = 0;
    double m_start = 0;
    double m_end = 0;
    double m_scale = 300;
    double m_da = 0;

};

} /* namespace agg */

#endif /* AGG_AGG_ARC_H_ */
