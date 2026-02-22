/*
 * PairInteger.h
 *
 *  Created on: May 22, 2018
 *      Author: florian
 */

#ifndef BASIC_PAIRINTEGER_H_
#define BASIC_PAIRINTEGER_H_

#include <boost/functional/hash/hash.hpp>
#include <cstddef>
#include <unordered_map>

namespace laby {
namespace basic {

// same as Intersection Class in Family.h
class PairInteger {

public:
    PairInteger(const std::size_t first = 0, const std::size_t second = 0) {
        if (first < second) {
            _first = first;
            _second = second;
        } else {
            _second = first;
            _first = second;
        }

    }

    bool operator==(const PairInteger& other) const {
        return (_first == other._first && _second == other._second);
    }

    void set_first(const std::size_t first) {
        _first = first;
    }

    void set_second(const std::size_t second) {
        _second = second;
    }

    std::size_t first() const {
        return _first;
    }

    std::size_t second() const {
        return _second;
    }

private:
    std::size_t _first;
    std::size_t _second;

};
} /* namespace basic */
} /* namespace laby */

namespace std {

template<>
struct hash<laby::basic::PairInteger> {
    std::size_t operator()(const laby::basic::PairInteger& c) const {
        // Start with a hash value of 0    .
        std::size_t seed = 0;

        // Compute individual hash values for first,
        // second and third and combine them using XOR
        // and bit shifting:
        boost::hash_combine(seed, hash<std::size_t>()(c.first()));
        boost::hash_combine(seed, hash<std::size_t>()(c.second()));

        // Return the result.
        return seed;

    }
};

} /* namespace std */

#endif /* BASIC_PAIRINTEGER_H_ */
