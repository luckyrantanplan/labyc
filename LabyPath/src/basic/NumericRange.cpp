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

auto NumericHelper::reduce(const int32_t& value, const int32_t& detailSize,
                           const int32_t& globalSize) -> std::optional<signed int> {
    int32_t const previous = ((value - 1) * globalSize) / detailSize;
    int32_t current = (value * globalSize) / detailSize;
    if (value == 0 || previous != current) {
        return current;
    }
    return std::nullopt;
}

} /* namespace laby */
