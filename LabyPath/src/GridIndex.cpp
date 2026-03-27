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

auto GridIndex::getIndexMap(const std::vector<Ribbon>& ribList)
    -> std::unordered_map<uint32_t, GridIndex> {
    std::unordered_map<uint32_t, GridIndex> mapOfGrids;
    for (std::size_t ribbonIndex = 0; ribbonIndex < ribList.size(); ++ribbonIndex) {
        // the skeleton grid info is on stroke color
        // put it on fill color, in order to make the arrangement with different circular, radial
        // info
        const Ribbon& ribbon = ribList.at(ribbonIndex);

        const uint32_t blue =
            laby::basic::Color::get_blue(static_cast<uint32_t>(ribbon.fillColor()));
        const uint32_t green =
            laby::basic::Color::get_green(static_cast<uint32_t>(ribbon.fillColor()));
        GridIndex& gridIndex = mapOfGrids.try_emplace(blue).first->second;
        switch (green) {
        case static_cast<uint32_t>(GridChannel::Circular): {
            gridIndex.setCircularIndex(ribbonIndex);
            break;
        }
        case static_cast<uint32_t>(GridChannel::Radial): {
            gridIndex.setRadialIndex(ribbonIndex);
            break;
        }
        case static_cast<uint32_t>(GridChannel::Limit): {
            gridIndex.setLimitIndex(ribbonIndex);
            break;
        }
        default:
            break;
        }
    }
    return mapOfGrids;
}

auto GridIndex::getArr(const std::vector<Ribbon>& ribList) const -> Arrangement_2 {

    if (!isValid()) {
        std::cout << "bad GridIndex\n";
        return {};
    }

    const Ribbon& ribCircular = ribList.at(circularIndex());
    const Ribbon& ribRadial = ribList.at(radialIndex());
    return Ribbon::createArr(ribCircular, ribRadial);
}

auto GridIndex::limit(const std::vector<Ribbon>& ribList) const -> const Ribbon& {
    return ribList.at(_limit);
}
} /* namespace laby */
