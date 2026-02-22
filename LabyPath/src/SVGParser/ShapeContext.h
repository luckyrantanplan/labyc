/*
 * ShapeContext2.h
 *
 *  Created on: Jun 18, 2018
 *      Author: florian
 */

#ifndef SVGPARSER_SHAPECONTEXT_H_
#define SVGPARSER_SHAPECONTEXT_H_

#include <svgpp/policy/marker_events.hpp>

#include <vector>

#include "../Ribbon.h"
#include "Context.h"

namespace laby {
namespace svgp {

class ShapeContext: public BaseContext {
public:
    ShapeContext(BaseContext & parent);

    void path_move_to(double x, double y, svgpp::tag::coordinate::absolute);

    void path_line_to(double x, double y, svgpp::tag::coordinate::absolute);

    void path_cubic_bezier_to(double x1, double y1, double x2, double y2, double x, double y, svgpp::tag::coordinate::absolute);

    void path_quadratic_bezier_to(double x1, double y1, double x, double y, svgpp::tag::coordinate::absolute);

    void path_elliptical_arc_to(double rx, double ry, double x_axis_rotation, bool large_arc_flag, bool sweep_flag, double x, double y, svgpp::tag::coordinate::absolute);

    void path_close_subpath();

    void path_exit();

    void marker(svgpp::marker_vertex v, double x, double y, double directionality, unsigned marker_index);

private:
    struct MarkerPos {
        svgpp::marker_vertex v;
        double x, y, directionality;
    };
    typedef std::vector<MarkerPos> Markers;
    Markers markers_;

    Ribbon* _currentRibbon = nullptr;

    color_t getColor(const Paint& paint);
};

} /* namespace svgp */
} /* namespace laby */

#endif /* SVGPARSER_SHAPECONTEXT_H_ */
