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

namespace laby {
namespace svgp {

class Loader {

public:

    Loader(const std::string& filename);

    const std::vector<Ribbon>& ribList() const {
        return _ribList;
    }
    std::vector<Ribbon>& ribList() {
        return _ribList;
    }

    const CGAL::Bbox_2& viewBox() const {
        return _viewBox;
    }

private:
    std::vector<Ribbon> _ribList;
    CGAL::Bbox_2 _viewBox;

};

} /* namespace svgp */
} /* namespace laby */

#endif /* SVGPARSER_LOADER_H_ */
