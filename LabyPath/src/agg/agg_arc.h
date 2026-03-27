/*
 * agg_arc.h
 *
 *  Created on: Jul 4, 2018
 *      Author: florian
 */

#ifndef AGG_AGG_ARC_H_
#define AGG_AGG_ARC_H_

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Point_2.h>
#include <cmath>
#include <vector>

namespace agg {

class Arc {
    using Kernel = CGAL::Exact_predicates_exact_constructions_kernel;
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
    using Point = CGAL::Point_2<Kernel>;

    Arc(const Point& centerPoint, double radius, const Point& startPoint, const Point& endPoint,
        bool isCounterClockwise = true);

    [[nodiscard]] auto getPoints() const -> const std::vector<Point>& {
        return _points;
    }

  private:
    struct ArcParameters {
        Point center;
        double radius = 0.0;
        double startAngle = 0.0;
        double endAngle = 0.0;
    };

    static constexpr double kDefaultScale = 300.0;

    std::vector<Point> _points;

    void init(const ArcParameters& parameters);

    void normalize(const ArcParameters& parameters);

    double _angle = 0.0;
    double _start = 0.0;
    double _end = 0.0;
    double _scale = kDefaultScale;
    double _deltaAngle = 0.0;
};

} /* namespace agg */

#endif /* AGG_AGG_ARC_H_ */
