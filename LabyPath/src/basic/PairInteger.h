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

namespace laby::basic {

// same as Intersection Class in Family.h
class PairInteger {

  public:
    explicit PairInteger(const std::size_t first = 0, const std::size_t second = 0) {
        if (first < second) {
            _first = first;
            _second = second;
        } else {
            _second = first;
            _first = second;
        }
    }

    auto operator==(const PairInteger& other) const -> bool {
        return (_first == other._first && _second == other._second);
    }

    void setFirst(const std::size_t first) {
        _first = first;
    }

    void setSecond(const std::size_t second) {
        _second = second;
    }

    [[nodiscard]] auto first() const -> std::size_t {
        return _first;
    }

    [[nodiscard]] auto second() const -> std::size_t {
        return _second;
    }

  private:
    std::size_t _first;
    std::size_t _second;
};
} // namespace laby::basic

namespace std {

template <> struct hash<laby::basic::PairInteger> {
    auto operator()(const laby::basic::PairInteger& c) const -> std::size_t {
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
