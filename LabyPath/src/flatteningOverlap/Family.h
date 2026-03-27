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
        } else {
            _second = first;
            _first = second;
        }
    }

    using Handle = CGAL::Union_find<std::size_t>::handle;

    auto operator==(const Intersection& other) const -> bool {
        return (_first == other._first && _second == other._second);
    }

    auto first() const -> std::size_t {
        return _first;
    }

    auto second() const -> std::size_t {
        return _second;
    }

    void setHandle(Handle unionFindHandle) const {
        _handle = unionFindHandle;
    }
    [[nodiscard]] auto handle() const -> const Handle& {
        return _handle;
    }

    void setFamilyHandle(Handle familyHandle) const {
        _familyHandle = familyHandle;
    }
    [[nodiscard]] auto familyHandle() const -> const Handle& {
        return _familyHandle;
    }

  private:
    std::size_t _first;
    std::size_t _second;
    mutable Handle _handle;
    mutable Handle _familyHandle;
};
} /* namespace laby */

namespace std {

template <> struct hash<laby::Intersection> {
    auto operator()(const laby::Intersection& intersection) const -> std::size_t {
        // Start with a hash value of 0    .
        std::size_t seed = 0;

        // Compute individual hash values for first,
        // second and third and combine them using XOR
        // and bit shifting:
        boost::hash_combine(seed, hash<std::size_t>()(intersection.first()));
        boost::hash_combine(seed, hash<std::size_t>()(intersection.second()));

        // Return the result.
        return seed;
    }
};

} // namespace std
namespace laby {
class Family {
  public:
    auto createPatch(const std::vector<PolyConvex>& polyConvexList) -> void;

    std::vector<Intersection> _intersections;

    std::unordered_map<std::size_t, std::vector<std::size_t>> _patches;
    mutable CGAL::Union_find<std::size_t>::handle handle;

    static void createUnionFind(const std::unordered_set<std::size_t>& coverSet,
                                const std::vector<PolyConvex>& polyConvexList,
                                CGAL::Union_find<std::size_t>& unionFind);

  private:
};

} /* namespace laby */

#endif /* FAMILY_H_ */
