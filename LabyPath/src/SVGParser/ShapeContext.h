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

namespace laby::svgp {

class ShapeContext : public BaseContext {
  public:
    explicit ShapeContext(BaseContext& parent);

    // SVG++ discovers these callbacks by exact name.
    
    void path_move_to(double xCoordinate, double yCoordinate,
                      svgpp::tag::coordinate::absolute coordinateTag);

    void path_line_to(double xCoordinate, double yCoordinate,
                      svgpp::tag::coordinate::absolute coordinateTag);

    void path_cubic_bezier_to(double xControl1, double yControl1, double xControl2,
                              double yControl2, double xCoordinate, double yCoordinate,
                              svgpp::tag::coordinate::absolute coordinateTag);

    void path_quadratic_bezier_to(double xControl, double yControl, double xCoordinate,
                                  double yCoordinate,
                                  svgpp::tag::coordinate::absolute coordinateTag);

    static void path_elliptical_arc_to(double radiusX, double radiusY, double xAxisRotation,
                                       bool largeArcFlag, bool sweepFlag, double xCoordinate,
                                       double yCoordinate,
                                       svgpp::tag::coordinate::absolute coordinateTag);

    void path_close_subpath();

    void path_exit();
    

    
    void marker(svgpp::marker_vertex vertex, double xCoordinate, double yCoordinate,
                double directionality, unsigned markerIndex);

  private:
    struct MarkerPos {
        svgpp::marker_vertex vertex;
        double xCoordinate;
        double yCoordinate;
        double directionality;
    };
    using Markers = std::vector<MarkerPos>;
    Markers _markers;

    auto currentRibbon() -> Ribbon&;

    auto getColor(const Paint& paint) -> color_t;
};

} /* namespace laby::svgp */

#endif /* SVGPARSER_SHAPECONTEXT_H_ */
