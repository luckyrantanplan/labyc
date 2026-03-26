/*
 * Context.h
 *
 *  Created on: Jun 15, 2018
 *      Author: florian
 */

#ifndef SVGPARSER_CONTEXT_H_
#define SVGPARSER_CONTEXT_H_

#include <boost/array.hpp>

#include <vector>

#include "../Ribbon.h"
#include "Stylable.h"

namespace laby {
namespace svgp {

class BaseContext: public Stylable {
public:

    BaseContext();

    BaseContext(BaseContext & parent);

    void on_exit_element() {
    }

    void transform_matrix(const boost::array<double, 6> & /*matrix*/) {
    }

    // Viewport Events Policy
    void set_viewport(double viewport_x, double viewport_y, double viewport_width, double viewport_height) {

        std::cout << "set viewport" << viewport_x << " " << viewport_y << " " << viewport_width << " " << viewport_height << std::endl;

    }

    void set_viewbox_size(double viewbox_width, double viewbox_height) {
        std::cout << "set_viewbox_size" << viewbox_width << " " << viewbox_height << std::endl;
        _viewbox=CGAL::Bbox_2(0,0,viewbox_width,viewbox_height);

    }

    void disable_rendering() {
    }
    std::vector<Ribbon> getRibbon() const {
        return _vectRibbonRef;
    }

    std::vector<Ribbon>& getRibbon() {
        return _vectRibbonRef;
    }

    const CGAL::Bbox_2& getViewbox() const {
        return _viewbox;
    }

    std::vector<Ribbon> _vectRibbon;
    std::vector<Ribbon>& _vectRibbonRef;
    CGAL::Bbox_2 _viewbox;
};

} /* namespace svgp */
} /* namespace laby */

#endif /* SVGPARSER_CONTEXT_H_ */
