/*
 * RandomUniDist2.h
 *
 *  Created on: May 11, 2018
 *      Author: florian
 */

#ifndef BASIC_RANDOMUNIDIST_H_
#define BASIC_RANDOMUNIDIST_H_

#include <cstdint>
#include <algorithm>
#include <random>

namespace laby {
namespace basic {

class RandomUniDist {

public:
    RandomUniDist(const double min, const double max, uint32_t seed = std::random_device()()) :
            gen { seed }, //
            dis { min, max } {

    }

    double get() {
        return dis(gen);
    }

    std::size_t select(std::size_t start, std::size_t end) {
        std::uniform_int_distribution<std::size_t> dis(start, end - 1);

        return dis(gen);
    }

    template<typename CONTAINER>
    void shuffle(CONTAINER& container) {
        std::shuffle(container.begin(), container.end(), gen);
    }

private:
    std::mt19937 gen;
    std::uniform_real_distribution<> dis;
};

} /* namespace basic */
} /* namespace laby */

#endif /* BASIC_RANDOMUNIDIST_H_ */
