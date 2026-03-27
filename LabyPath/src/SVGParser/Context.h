/*
 * Context.h
 *
 *  Created on: Jun 15, 2018
 *      Author: florian
 */

#ifndef SVGPARSER_CONTEXT_H_
#define SVGPARSER_CONTEXT_H_

#include <cstddef>

#include <boost/array.hpp>

#include <vector>

#include "../Ribbon.h"
#include "Stylable.h"

namespace laby::svgp {

inline constexpr std::size_t kTransformMatrixSize = 6;

class BaseContext : public Stylable {
  public:
    BaseContext();
    ~BaseContext() = default;

    BaseContext(BaseContext& parent);

    BaseContext(const BaseContext& parent);

    auto operator=(const BaseContext&) -> BaseContext& = delete;
    BaseContext(BaseContext&&) = delete;
    auto operator=(BaseContext&&) -> BaseContext& = delete;

    // SVG++ discovers these hooks by exact name.
    
    void on_exit_element() {}

    void transform_matrix(const boost::array<double, kTransformMatrixSize>& /*matrix*/) {}

    // Viewport Events Policy
    static void set_viewport(double viewport_x, double viewport_y, double viewport_width,
                             double viewport_height) {

        std::cout << "set viewport" << viewport_x << " " << viewport_y << " " << viewport_width
                  << " " << viewport_height << '\n';
    }

    void set_viewbox_size(double viewbox_width, double viewbox_height) {
        std::cout << "set_viewbox_size" << viewbox_width << " " << viewbox_height << '\n';
        _viewbox = CGAL::Bbox_2(0, 0, viewbox_width, viewbox_height);
    }

    void disable_rendering() {}
    

    [[nodiscard]] auto getRibbon() const -> const std::vector<Ribbon>& {
        return *_vectRibbonRef;
    }

    
    auto getRibbon() -> std::vector<Ribbon>& {
        return *_vectRibbonRef;
    }

    [[nodiscard]] auto getViewbox() const -> const CGAL::Bbox_2& {
        return _viewbox;
    }

    std::vector<Ribbon> _vectRibbon;
    std::vector<Ribbon>* _vectRibbonRef;
    CGAL::Bbox_2 _viewbox;
};

} /* namespace laby::svgp */

#endif /* SVGPARSER_CONTEXT_H_ */
