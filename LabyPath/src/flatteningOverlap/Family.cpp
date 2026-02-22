/*
 * Family.cpp
 *
 *  Created on: Mar 15, 2018
 *      Author: florian
 */

#include "Family.h"

#include <CGAL/Box_intersection_d/Box_with_handle_d.h>
#include "basic/EasyProfilerCompat.h"
#include <cstdlib>
#include <iostream>
#include <utility>

namespace laby {

typedef CGAL::Box_intersection_d::Box_with_handle_d<double, 2, const Family*> BoxFamily;

void Family::createUnionFind(const std::unordered_set<std::size_t>& coverSet, const std::vector<PolyConvex>& polyConvexList, CGAL::Union_find<std::size_t> &uf) {
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
    } else {
        std::cout << " not found " << i;
    }
}

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

    } else if (uf.number_of_sets() == 1) {
        auto ite = _patches.try_emplace(uf.begin().ptr()->value, std::vector<std::size_t>());
        for (const std::size_t& i : coverSet) {
            ite.first->second.emplace_back(i);
        }

    } else if (uf.number_of_sets() > 2) {
        std::cout << " too much patch " << std::endl;
        exit(-1);
    }

}
} /* namespace laby */
