/*
 * GridIndex.cpp
 *
 *  Created on: Sep 25, 2018
 *      Author: florian
 */

#include "GridIndex.h"

#include <utility>

#include "basic/Color.h"

namespace laby {

std::unordered_map<uint32_t, GridIndex> GridIndex::getIndexMap(const std::vector<Ribbon>& ribList) {
    std::unordered_map<uint32_t, GridIndex> mapOfGrids;
    for (std::size_t i = 0; i < ribList.size(); ++i) {
        // the skeleton grid info is on stroke color
        // put it on fill color, in order to make the arrangement with different circular, radial info
        const Ribbon& rib = ribList.at(i);

        const uint32_t blue = laby::basic::Color::get_blue(static_cast<uint32_t>(rib.fill_color()));
        const uint32_t green = laby::basic::Color::get_green(static_cast<uint32_t>(rib.fill_color()));
        GridIndex& gridIndex = mapOfGrids.try_emplace(blue).first->second;
        switch (green) {
        case static_cast<uint32_t>(GridChannel::Circular): {
            gridIndex._circular = i;
            break;
        }
        case static_cast<uint32_t>(GridChannel::Radial): {
            gridIndex._radial = i;
            break;
        }
        case static_cast<uint32_t>(GridChannel::Limit): {
            gridIndex._limit = i;
            break;
        }
        }
    }
    return mapOfGrids;
}

Arrangement_2 GridIndex::getArr(const std::vector<Ribbon>& ribList) const {

    if (!isValid()) {
        std::cout << "bad GridIndex" << std::endl;
        return Arrangement_2();
    } else {
        const Ribbon& ribCircular = ribList.at(_circular);
        const Ribbon& ribRadial = ribList.at(_radial);
        return Ribbon::createArr(ribCircular, ribRadial);
    }
}
const Ribbon& GridIndex::limit(const std::vector<Ribbon>& ribList) const {
    return ribList.at(_limit);

}
} /* namespace laby */
