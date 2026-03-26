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

#include <CGAL/Box_intersection_d/Box_with_handle_d.h>
#include "basic/EasyProfilerCompat.h"
#include <iostream>
#include <stdexcept>
#include <utility>

namespace laby {

typedef CGAL::Box_intersection_d::Box_with_handle_d<double, 2, const Family*> BoxFamily;

/**
 * Build a Union-Find structure over the polygon indices in @p coverSet,
 * merging any two indices that are adjacent in @p polyConvexList.
 */
void Family::createUnionFind(const std::unordered_set<std::size_t>& coverSet, const std::vector<PolyConvex>& polyConvexList,
                             CGAL::Union_find<std::size_t>& uf) {
    EASY_FUNCTION();
    for (const std::size_t& i : coverSet) {
        polyConvexList.at(i).handle = uf.push_back(i);
    }
    for (const std::size_t& i : coverSet) {
        const PolyConvex& pc = polyConvexList.at(i);

        for (std::size_t a : pc._adjacents) {
            if (coverSet.count(a) > 0) {
                uf.unify_sets(pc.handle, polyConvexList.at(a).handle);
            }
        }
    }
}

void Family::printFind(const std::unordered_set<std::size_t>& coverSet, std::size_t i) {
    if (coverSet.count(i) > 0) {
        std::cout << " find " << i;
    }
    else {
        std::cout << " not found " << i;
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
void Family::createPatch(const std::vector<PolyConvex>& polyConvexList) {
    EASY_FUNCTION();
    std::unordered_set<std::size_t> coverSet;
    for (Intersection& i : _intersections) {
        coverSet.emplace(i.first());
        coverSet.emplace(i.second());
    }

    CGAL::Union_find<std::size_t> uf;
    createUnionFind(coverSet, polyConvexList, uf);
    if (uf.number_of_sets() == 2) {
        for (const std::size_t& i : coverSet) {
            const PolyConvex& pc = polyConvexList.at(i);
            const std::size_t& head = uf.find(pc.handle).ptr()->value;

            auto ite = _patches.try_emplace(head, std::vector<std::size_t>());
            ite.first->second.emplace_back(i);
        }
    }
    else if (uf.number_of_sets() == 1) {
        auto ite = _patches.try_emplace(uf.begin().ptr()->value, std::vector<std::size_t>());
        for (const std::size_t& i : coverSet) {
            ite.first->second.emplace_back(i);
        }
    }
    else if (uf.number_of_sets() > 2) {
        throw std::runtime_error("Family::createPatch: unexpected topology – more than 2 patches ("
                                 + std::to_string(uf.number_of_sets()) + " found)");
    }
}
} /* namespace laby */
