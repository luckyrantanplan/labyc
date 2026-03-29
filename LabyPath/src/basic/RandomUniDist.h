/*
 * RandomUniDist2.h
 *
 *  Created on: May 11, 2018
 *      Author: florian
 */

#ifndef BASIC_RANDOMUNIDIST_H_
#define BASIC_RANDOMUNIDIST_H_

#include <algorithm>
#include <cstdint>
#include <random>

namespace laby::basic {

class RandomUniDist {

  public:
    RandomUniDist(const double min, const double max, uint32_t seed = std::random_device()())
        : _gen{seed}, //
          _dis{min, max} {}

    auto get() -> double {
        return _dis(_gen);
    }

    auto select(std::size_t start, std::size_t end) -> std::size_t {
        std::uniform_int_distribution<std::size_t> udis(start, end - 1);

        return udis(_gen);
    }

    template <typename CONTAINER> void shuffle(CONTAINER& container) {
        std::shuffle(container.begin(), container.end(), _gen);
    }

  private:
    std::mt19937 _gen;
    std::uniform_real_distribution<> _dis{};
};

} // namespace laby::basic

#endif /* BASIC_RANDOMUNIDIST_H_ */
