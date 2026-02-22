/*
 * NumericRange.cpp
 *
 *  Created on: Feb 20, 2018
 *      Author: florian
 */

#include "NumericRange.h"

#include <cstdint>
#include <optional>

namespace laby {

std::optional<signed int> NumericHelper::reduce(const int32_t& x, const int32_t& detailSize, const int32_t& globalSize) {
    int32_t previous = ((x - 1) * globalSize) / detailSize;
    int32_t current = (x * globalSize) / detailSize;
    if (x == 0 || previous != current) {
        return current;
    }
    return std::nullopt;
}

} /* namespace laby */
