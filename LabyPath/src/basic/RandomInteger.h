/*
 * RandomInteger.h
 *
 *  Created on: May 9, 2018
 *      Author: florian
 */

#ifndef BASIC_RANDOMINTEGER_H_
#define BASIC_RANDOMINTEGER_H_

#include <bits/stdint-uintn.h>
#include <random>

namespace laby {
namespace basic {

template<typename NUMBER> class RandomInteger {

public:
    RandomInteger(const NUMBER min, const NUMBER max, uint32_t seed = std::random_device()()) :
            gen { seed }, //
            dis { min, max } {

    }

    NUMBER get() {
        return dis(gen);
    }

private:
    std::mt19937 gen;
    std::uniform_int_distribution<NUMBER> dis;

};

} /* namespace basic */
} /* namespace laby */

#endif /* BASIC_RANDOMINTEGER_H_ */
