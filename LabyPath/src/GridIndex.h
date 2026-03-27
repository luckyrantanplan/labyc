/*
 * GridIndex.h
 *
 *  Created on: Sep 25, 2018
 *      Author: florian
 */

#ifndef GRIDINDEX_H_
#define GRIDINDEX_H_

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "Ribbon.h"

namespace laby {

/// Grid channel type, encoded in the green channel of the ribbon fill color.
enum class GridChannel : std::uint8_t {
    Circular = 50, ///< Circular (concentric) skeleton path
    Radial = 100,  ///< Radial (spoke) skeleton path
    Limit = 150    ///< Boundary limit path
};

class GridIndex {
  public:
    static auto
    getIndexMap(const std::vector<Ribbon>& ribList) -> std::unordered_map<uint32_t, GridIndex>;
    [[nodiscard]] auto getArr(const std::vector<Ribbon>& ribList) const -> Arrangement_2;

    [[nodiscard]] auto limit(const std::vector<Ribbon>& ribList) const -> const Ribbon&;

    [[nodiscard]] auto isValid() const -> bool {
        return (_circular != _radial) and (_radial != _limit) and (_circular != _limit);
    }

    auto setCircularIndex(const std::size_t circularIndex) -> void {
        _circular = circularIndex;
    }
    auto setRadialIndex(const std::size_t radialIndex) -> void {
        _radial = radialIndex;
    }
    auto setLimitIndex(const std::size_t limitIndex) -> void {
        _limit = limitIndex;
    }

    [[nodiscard]] auto circularIndex() const -> std::size_t {
        return _circular;
    }
    [[nodiscard]] auto radialIndex() const -> std::size_t {
        return _radial;
    }

  private:
    std::size_t _circular = 0;
    std::size_t _radial = 0;
    std::size_t _limit = 0;
};

} /* namespace laby */

#endif /* GRIDINDEX_H_ */
