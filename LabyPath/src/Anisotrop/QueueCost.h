/*
 * QueueCost.h
 *
 *  Created on: Apr 26, 2018
 *      Author: florian
 */

#ifndef ANISOTROP_QUEUECOST_H_
#define ANISOTROP_QUEUECOST_H_

#include <cstdint>
#include <iostream>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

#include "../GeomData.h"

namespace laby::aniso {

class QueueCost {
  public:
    auto distance() -> int32_t& {
        return _distance;
    }

    [[nodiscard]] auto distance() const -> int32_t {
        return _distance;
    }

    auto viaNum() -> int32_t& {
        return _viaNum;
    }

    [[nodiscard]] auto viaNum() const -> int32_t {
        return _viaNum;
    }

    auto congestion() -> int32_t& {
        return _congestion;
    }

    [[nodiscard]] auto congestion() const -> int32_t {
        return _congestion;
    }

    auto randomization() -> int32_t& {
        return _randomization;
    }

    [[nodiscard]] auto randomization() const -> int32_t {
        return _randomization;
    }

    auto memorySource() -> std::unordered_set<int32_t>& {
        return _memorySource;
    }

    [[nodiscard]] auto memorySource() const -> const std::unordered_set<int32_t>& {
        return _memorySource;
    }

    auto futureMemorySource() -> std::unordered_set<int32_t>& {
        return _futureMemorySource;
    }

    [[nodiscard]] auto futureMemorySource() const -> const std::unordered_set<int32_t>& {
        return _futureMemorySource;
    }

    auto memoryTarget() -> std::unordered_set<int32_t>& {
        return _memoryTarget;
    }

    [[nodiscard]] auto memoryTarget() const -> const std::unordered_set<int32_t>& {
        return _memoryTarget;
    }

    auto futureMemoryTarget() -> std::unordered_set<int32_t>& {
        return _futureMemoryTarget;
    }

    [[nodiscard]] auto futureMemoryTarget() const -> const std::unordered_set<int32_t>& {
        return _futureMemoryTarget;
    }

    auto tie() const {
        return std::tie(_congestion, //

                        _viaNum,  //
                        _distance, //
                        _randomization);
    }

    void print(std::ostream& outputStream) const {
        outputStream << " congestion " << _congestion;
        outputStream << " via_num " << _viaNum;
        outputStream << "distance " << _distance;
        outputStream << " randomization " << _randomization;
    }

    auto operator>(const QueueCost& rhs) const -> bool {
        return tie() > rhs.tie();
    }

    auto operator<(const QueueCost& rhs) const -> bool {
        return tie() < rhs.tie();
    }

    private:
        int32_t _distance = -1; // Distance from source to current element
        int32_t _viaNum = 0;    // Via count from source to current element
        int32_t _congestion = 0;

        int32_t _randomization = 0;
        std::unordered_set<int32_t> _memorySource;
        std::unordered_set<int32_t> _futureMemorySource;
        std::unordered_set<int32_t> _memoryTarget;
        std::unordered_set<int32_t> _futureMemoryTarget;
};

} // namespace laby::aniso

#endif /* ANISOTROP_QUEUECOST_H_ */
