/**
 * @file Family.cpp
 * @brief Groups overlapping polygon pairs into connected families using Union-Find.
 *
 * A Family represents a cluster of Intersection pairs whose polygons are
 * transitively connected through adjacency.  createPatch() further splits each
 * family into 1 or 2 *patches* (connected components of polygon indices).
 *
 * Two patches → clean two-sided overlap (each side rendered separately).
 * One patch   → single-piece overlap (needs special handling in PathRendering).
 */

#include "Family.h"

#include "basic/EasyProfilerCompat.h"
#include <stdexcept>
#include <string>

namespace laby {

/**
 * Build a Union-Find structure over the polygon indices in @p coverSet,
 * merging any two indices that are adjacent in @p polyConvexList.
 */
static void Family::createUnionFind(const std::unordered_set<std::size_t>& coverSet,
                                    const std::vector<PolyConvex>& polyConvexList,
                                    CGAL::Union_find<std::size_t>& unionFind) {
    EASY_FUNCTION();
    for (const std::size_t& coverIndex : coverSet) {
        polyConvexList.at(coverIndex).handle = unionFind.push_back(coverIndex);
    }
    for (const std::size_t& coverIndex : coverSet) {
        const PolyConvex& polyConvex = polyConvexList.at(coverIndex);

        for (std::size_t const adjacentIndex : polyConvex._adjacents) {
            if (coverSet.count(adjacentIndex) > 0) {
                unionFind.unify_sets(polyConvex.handle, polyConvexList.at(adjacentIndex).handle);
            }
        }
    }
}

/**
 * Split this family's polygon indices into connected components (patches).
 *
 * Expected results:
 *  - 2 patches: the standard two-sided overlap (each patch → one node)
 *  - 1 patch:   all polygons are connected (single-piece overlap)
 *  - >2 patches: unexpected topology – throws std::runtime_error
 */
static auto Family::createPatch(const std::vector<PolyConvex>& polyConvexList) -> void {
    EASY_FUNCTION();
    std::unordered_set<std::size_t> coverSet;
    for (Intersection const& intersection : _intersections) {
        coverSet.emplace(intersection.first());
        coverSet.emplace(intersection.second());
    }

    CGAL::Union_find<std::size_t> unionFind;
    createUnionFind(coverSet, polyConvexList, unionFind);
    if (unionFind.number_of_sets() == 2) {
        for (const std::size_t& coverIndex : coverSet) {
            const PolyConvex& polyConvex = polyConvexList.at(coverIndex);
            const std::size_t& head = unionFind.find(polyConvex.handle).ptr()->value;

            auto iterator = _patches.try_emplace(head, std::vector<std::size_t>());
            iterator.first->second.emplace_back(coverIndex);
        }
    } else if (unionFind.number_of_sets() == 1) {
        auto iterator =
            _patches.try_emplace(unionFind.begin().ptr()->value, std::vector<std::size_t>());
        for (const std::size_t& coverIndex : coverSet) {
            iterator.first->second.emplace_back(coverIndex);
        }
    } else if (unionFind.number_of_sets() > 2) {
        throw std::runtime_error(
            "Family::createPatch: unexpected topology – more than 2 patches (" +
            std::to_string(unionFind.number_of_sets()) + " found)");
    }
}
} /* namespace laby */
