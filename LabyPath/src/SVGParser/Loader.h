/*
 * Loader.h
 *
 *  Created on: Jun 15, 2018
 *      Author: florian
 */

#ifndef SVGPARSER_LOADER_H_
#define SVGPARSER_LOADER_H_

#include <string>
#include <vector>

#include "../Ribbon.h"

namespace laby::svgp {

class Loader {

  public:
    explicit Loader(const std::string& filename);

    [[nodiscard]] auto ribList() const -> const std::vector<Ribbon>& {
        return _ribList;
    }
    auto ribList() -> std::vector<Ribbon>& {
        return _ribList;
    }

    [[nodiscard]] auto viewBox() const -> const CGAL::Bbox_2& {
        return _viewBox;
    }

  private:
    std::vector<Ribbon> _ribList;
    CGAL::Bbox_2 _viewBox;
};

} // namespace laby::svgp

#endif /* SVGPARSER_LOADER_H_ */
