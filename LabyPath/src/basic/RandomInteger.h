/*
 * RandomInteger.h
 *
 *  Created on: May 9, 2018
 *      Author: florian
 */

#ifndef BASIC_RANDOMINTEGER_H_
#define BASIC_RANDOMINTEGER_H_

#include <cstdint>
#include <random>


namespace laby::basic {

template<typename NUMBER> class RandomInteger {

public:
    RandomInteger(const NUMBER min, const NUMBER max, uint32_t seed = std::random_device()()) :
            _gen { seed }, //
            _dis { min, max } {

    }

    auto get() -> NUMBER {
        return _dis(_gen);
    }

private:
    std::mt19937 _gen;
    std::uniform_int_distribution<NUMBER> _dis;

};

} // namespace laby::basic


#endif /* BASIC_RANDOMINTEGER_H_ */
