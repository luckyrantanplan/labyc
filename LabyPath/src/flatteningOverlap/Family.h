/*
 * Family.h
 *
 *  Created on: Mar 15, 2018
 *      Author: florian
 */

#ifndef FAMILY_H_
#define FAMILY_H_

#include <boost/functional/hash/hash.hpp>

#include <CGAL/Union_find.h>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../PolyConvex.h"

namespace laby {

class Intersection {
public:
    Intersection(const std::size_t first, const std::size_t second) {
        if (first < second) {
            _first = first;
            _second = second;
        }
        else {
            _second = first;
            _first = second;
        }
    }

    bool operator==(const Intersection& other) const { return (_first == other._first && _second == other._second); }

    std::size_t first() const { return _first; }

    std::size_t second() const { return _second; }

    mutable CGAL::Union_find<std::size_t>::handle handle;
    mutable CGAL::Union_find<std::size_t>::handle family_handle;

private:
    std::size_t _first;
    std::size_t _second;
};
} /* namespace laby */

namespace std {

template <> struct hash<laby::Intersection> {
    std::size_t operator()(const laby::Intersection& c) const {
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

} // namespace std
namespace laby {
class Family {
public:
    void createPatch(const std::vector<PolyConvex>& polyConvexList);

    std::vector<Intersection> _intersections;

    std::unordered_map<std::size_t, std::vector<std::size_t>> _patches;
    mutable CGAL::Union_find<std::size_t>::handle handle;

    static void createUnionFind(const std::unordered_set<std::size_t>& coverSet, const std::vector<PolyConvex>& polyConvexList,
                                CGAL::Union_find<std::size_t>& uf);
    void printFind(const std::unordered_set<std::size_t>& coverSet, std::size_t i);

private:
};

} /* namespace laby */

#endif /* FAMILY_H_ */
