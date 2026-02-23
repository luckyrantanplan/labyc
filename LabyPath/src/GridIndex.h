/*
 * GridIndex.h
 *
 *  Created on: Sep 25, 2018
 *      Author: florian
 */

#ifndef GRIDINDEX_H_
#define GRIDINDEX_H_

#include <cstdint>
#include <cstddef>
#include <unordered_map>
#include <vector>

#include "Ribbon.h"

namespace laby {

/// Grid channel type, encoded in the green channel of the ribbon fill color.
enum class GridChannel : uint32_t {
    Circular = 50,   ///< Circular (concentric) skeleton path
    Radial   = 100,  ///< Radial (spoke) skeleton path
    Limit    = 150   ///< Boundary limit path
};

class GridIndex {
public:
    static std::unordered_map<uint32_t, GridIndex> getIndexMap(const std::vector<Ribbon>& ribList);
    Arrangement_2 getArr(const std::vector<Ribbon>& ribList) const;

    const Ribbon& limit(const std::vector<Ribbon>& ribList) const;

    bool isValid() const {
        return (_circular != _radial) and (_radial != _limit) and (_circular != _limit);
    }

    std::size_t _circular;
    std::size_t _radial;
private:
    std::size_t _limit;

};

} /* namespace laby */

#endif /* GRIDINDEX_H_ */
