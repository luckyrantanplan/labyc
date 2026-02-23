/*
 * PathRendering.cpp
 *
 *  Created on: Mar 8, 2018
 *      Author: florian
 */

#include "PathRendering.h"

#include <cstdint>
#include <CGAL/Arrangement_on_surface_2.h>
#include <CGAL/box_intersection_d.h>
#include <CGAL/Bbox_2.h>
#include <CGAL/Boolean_set_operations_2/Gps_on_surface_base_2.h>

#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_set_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include "basic/EasyProfilerCompat.h"
#include <algorithm>
#include <iostream>
#include <iterator>
#include <queue>
#include <utility>

#include "../basic/PolygonTools.h"
#include "../OrientedRibbon.h"
#include "NodeRendering.h"

namespace laby {

typedef CGAL::Box_intersection_d::Box_with_handle_d<double, 2, const PolyConvex*> BoxPolyConvex;

void PathRendering::unify(std::size_t second, const std::vector<std::size_t>& aAdjs, std::unordered_set<Intersection>& intersections,
                          CGAL::Union_find<size_t>& uf, const Intersection& i) {

    EASY_FUNCTION();
    for (std::size_t a : aAdjs) {

        std::unordered_set<Intersection>::const_iterator ite = intersections.find({a, second});
        if (ite != intersections.end()) {
            uf.unify_sets(i.handle, ite->handle);
        }
    }
}

void PathRendering::createUnion(OrientedRibbon& ribs, const std::vector<PolyConvex>& polyConvexList) {
    EASY_FUNCTION();
    CGAL::Polygon_set_2<Kernel> set;

    std::vector<Linear_polygon> plv;
    plv.reserve(polyConvexList.size());
    for (const PolyConvex& pc : polyConvexList) {
        plv.emplace_back(pc._geometry);
    }
    set.join(plv.begin(), plv.end());

    std::vector<CGAL::Polygon_with_holes_2<Kernel>> polygons;

    set.polygons_with_holes(std::back_inserter(polygons));

    for (const CGAL::Polygon_with_holes_2<Kernel>& polygon : polygons) {
        for (const Linear_polygon::Segment_2& seg : RangeHelper::make(polygon.outer_boundary().edges_begin(), polygon.outer_boundary().edges_end())) {
            ribs.addCCW(seg);
        }

        for (const auto& hole : RangeHelper::make(polygon.holes_begin(), polygon.holes_end())) {
            for (const Linear_polygon::Segment_2& seg : RangeHelper::make(hole.edges_begin(), hole.edges_end())) {
                ribs.addCW(seg);
            }
        }
    }
}

bool PathRendering::do_intersect(Intersection& ai, Intersection& bi, const std::vector<PolyConvex>& polyConvexList) {
    EASY_FUNCTION();
    const Linear_polygon& lpb1 = polyConvexList.at(bi.first())._geometry;
    const Linear_polygon& lpb2 = polyConvexList.at(bi.second())._geometry;
    const Linear_polygon& lpa1 = polyConvexList.at(ai.first())._geometry;
    const Linear_polygon& lpa2 = polyConvexList.at(ai.second())._geometry;
    // works only on convex

    if (PolyConvex::testConvexPolyIntersect(lpa1, lpb1)) {
        if (PolyConvex::testConvexPolyIntersect(lpa1, lpb2)) {
            if (PolyConvex::testConvexPolyIntersect(lpa2, lpb1)) {
                if (PolyConvex::testConvexPolyIntersect(lpa2, lpb2)) {

                    CGAL::Polygon_set_2<Kernel> set(lpb2);
                    set.intersection(lpa1);
                    set.intersection(lpa2);

                    std::vector<CGAL::Polygon_with_holes_2<Kernel>> bigPolygons_a;
                    set.polygons_with_holes(std::back_inserter(bigPolygons_a));

                    for (const CGAL::Polygon_with_holes_2<Kernel>& polyHole : bigPolygons_a) {
                        const Linear_polygon& lpa = polyHole.outer_boundary();

                        if (PolyConvex::testConvexPolyIntersect(lpa, lpb1)) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

std::unordered_map<std::size_t, std::vector<const Family*>>
PathRendering::locateFamilies(const std::unordered_map<std::size_t, std::size_t>& families, std::vector<Family>& familyVector,
                              std::vector<Intersection>& intersections, std::vector<BoxIntersection>& box_intersection, const CGAL::Union_find<std::size_t>& uf,
                              const std::vector<PolyConvex>& polyConvexList) {

    EASY_FUNCTION();
    CGAL::Union_find<std::size_t> uf_family;

    for (auto ite_head : families) {
        std::size_t i = ite_head.first;
        intersections.at(i).family_handle = uf_family.push_back(i);
    }
    CGAL::box_self_intersection_d(box_intersection.begin(), box_intersection.end(), [&](const BoxIntersection& a, const BoxIntersection& b) {
        Intersection ai = intersections.at(a.handle());
        Intersection bi = intersections.at(b.handle());

        // reference the family through the index of example Intersection

        std::size_t head_ai = *uf.find(ai.handle);

        std::size_t head_bi = *uf.find(bi.handle);
        if (head_ai != head_bi) { // not the same family
            Intersection hai = intersections.at(head_ai);
            Intersection hbi = intersections.at(head_bi);
            if (!uf_family.same_set(hai.family_handle, hbi.family_handle)) { // not already in the same group of family
                bool result = do_intersect(ai, bi, polyConvexList);
                if (result) {
                    uf_family.unify_sets(hai.family_handle, hbi.family_handle);
                }
            }
        }
    });
    std::unordered_map<std::size_t, std::vector<const Family*>> families2;

    for (auto ite_head : families) {
        std::size_t i = ite_head.first;
        std::size_t head = *uf_family.find(intersections.at(i).family_handle);
        auto ite = families2.try_emplace(head);
        ite.first->second.emplace_back(&familyVector.at(ite_head.second));
    }

    return families2;
}

void PathRendering::createIntersect(OrientedRibbon& oribbon, const std::vector<PolyConvex>& polyConvexList) {
    EASY_FUNCTION();
    std::cout << "start intersect" << std::endl;

    std::vector<Intersection> intersections;
    intersections.reserve(polyConvexList.size());
    std::vector<BoxPolyConvex> boxes;
    boxes.reserve(polyConvexList.size());
    for (const PolyConvex& pc : polyConvexList) {
        boxes.emplace_back(pc._geometry.bbox(), &pc);
    }

    std::cout << "start boxes  boxes size " << boxes.size() << std::endl;

    std::vector<BoxIntersection> box_intersection;
    box_intersection.reserve(boxes.size()); // reserve based on boxes, not empty intersections

    CGAL::box_self_intersection_d(boxes.begin(), boxes.end(), [&](const BoxPolyConvex& a, const BoxPolyConvex& b) {
        const std::vector<std::size_t>& adjacents = a.handle()->_adjacents;
        std::vector<std::size_t>::const_iterator ite = std::find(adjacents.begin(), adjacents.end(), b.handle()->_id);
        if (ite != adjacents.end()) {
            intersections.emplace_back(a.handle()->_id, b.handle()->_id);
        }
        else {

            if (PolyConvex::testConvexPolyIntersect(a.handle()->_geometry, b.handle()->_geometry)) {
                intersections.emplace_back(a.handle()->_id, b.handle()->_id);
                CGAL::Bbox_2 ba = a.handle()->_geometry.bbox();
                CGAL::Bbox_2 bb = b.handle()->_geometry.bbox();

                double xmin = std::max(ba.xmin(), bb.xmin());
                double xmax = std::min(ba.xmax(), bb.xmax());
                double ymin = std::max(ba.ymin(), bb.ymin());
                double ymax = std::min(ba.ymax(), bb.ymax());
                CGAL::Bbox_2 ibox(xmin, ymin, xmax, ymax);

                box_intersection.emplace_back(ibox, intersections.size() - 1);
            }
        }
    });
    std::cout << "end boxes" << std::endl;
    CGAL::Union_find<std::size_t> uf;

    for (std::size_t i = 0; i < intersections.size(); ++i) {
        intersections.at(i).handle = uf.push_back(i);
    }

    std::unordered_set<Intersection> intersections_set(intersections.begin(), intersections.end());
    for (const Intersection& i : intersections) {
        const std::vector<std::size_t>& aAdjs = polyConvexList.at(i.first())._adjacents;
        const std::vector<std::size_t>& bAdjs = polyConvexList.at(i.second())._adjacents;

        unify(i.second(), aAdjs, intersections_set, uf, i);
        unify(i.first(), bAdjs, intersections_set, uf, i);
    }

    std::cout << "unify" << std::endl;

    std::unordered_map<std::size_t, std::size_t> families;
    std::vector<Family> familyVector;
    familyVector.reserve(intersections.size());
    for (const Intersection& i : intersections) {

        std::size_t head = *uf.find(i.handle);
        auto ite = families.try_emplace(head, familyVector.size());
        if (ite.second) {
            familyVector.emplace_back();
        }
        familyVector.at(ite.first->second)._intersections.emplace_back(i);
    }

    std::vector<Node> nodes = processFamilies(families, polyConvexList, familyVector, intersections, box_intersection, uf);
    std::cout << "processFamilies" << std::endl;
    chooseNodeState(nodes);
    std::cout << "chooseNodeState" << std::endl;
    NodeRendering::render(oribbon, nodes, polyConvexList);
}

void PathRendering::nodeAdjacence(std::vector<Node>& nodes, const std::vector<PolyConvex>& polyConvexList) {
    // compute adjacence
    EASY_FUNCTION();
    for (Node& n : nodes) {
        for (std::size_t i : n._cover) {
            polyConvexList.at(i)._nodes.emplace_back(&n);
        }
    }
    // we simplify to have only 1 node per PolyConvex
    for (const PolyConvex& pc : polyConvexList) {
        std::vector<Node*>& pcNodes = pc._nodes;
        while (pcNodes.size() > 1) {
            Node& n = *pcNodes.back();
            pcNodes.resize(pcNodes.size() - 1u);
            for (Node* m : pcNodes) {
                n._adjacents.emplace(m);
                m->_adjacents.emplace(&n);
            }
        }
    }
    for (Node& n : nodes) {

        std::vector<std::size_t>& cover = n._cover;
        // first find a node
        std::queue<QueueNodePolyConvex> queue;
        for (std::size_t i : cover) {
            const PolyConvex& pc = polyConvexList.at(i);
            if (pc._visited != -1) {
                queue.emplace(n, polyConvexList.at(i));
                break;
            }
        }
        while (!queue.empty()) {
            QueueNodePolyConvex pair = queue.front();
            queue.pop();
            pair._pc._visited = -1;
            const PolyConvex& pc = pair._pc;
            if (pc._nodes.empty() || pc._nodes.at(0) == &pair._n) {
                for (std::size_t i : pc._adjacents) {
                    if (polyConvexList.at(i)._visited != -1) {
                        queue.emplace(pair._n, polyConvexList.at(i));
                    }
                }
            }
            else {
                Node& newNode = *pc._nodes.at(0);
                newNode._adjacents.emplace(&pair._n);
                pair._n._adjacents.emplace(&newNode);
                for (std::size_t i : pc._adjacents) {
                    if (polyConvexList.at(i)._visited != -1) {
                        queue.emplace(newNode, polyConvexList.at(i));
                    }
                }
            }
        }
    }
}

void PathRendering::chooseNodeState(std::vector<Node>& nodes) {
    EASY_FUNCTION();
    for (Node& beginNode : nodes) {

        if (beginNode._state == -1) {
            {
                StateSelect stateSelect(beginNode._opposite);
                beginNode.setState(stateSelect.getNext());
            }

            std::priority_queue<NodeQueue> queue;
            queue.emplace(beginNode);
            while (!queue.empty()) {
                Node& node = queue.top().node();
                queue.pop();
                int32_t state = node._state;

                StateSelect stateSelect(node._opposite);
                stateSelect.markOccupied(state);

                for (Node* opp : node._opposite) {
                    int32_t& ostate = opp->_state;

                    if (ostate == -1) {
                        ostate = stateSelect.getNext();
                        queue.emplace(*opp);
                    }
                }
                int32_t adjstate;
                if (state == 0) {
                    adjstate = 1;
                }
                else {
                    adjstate = 0;
                }
                for (Node* adj : node._adjacents) {

                    int32_t& ostate = adj->_state;
                    if (ostate == -1 and !adj->haveOppositeState()) {
                        ostate = adjstate;
                        queue.emplace(*adj);
                    }
                }
            }
        }
    }
}

std::vector<Node> PathRendering::createNode(const std::unordered_map<std::size_t, std::vector<const Family*>>& map,
                                            std::unordered_set<Intersection> intersectOnSinglePiece, const std::vector<PolyConvex>& polyConvexList) {
    EASY_FUNCTION();
    // we avoid re allocation to preserve pointer
    std::vector<Node> nodes;
    {
        std::size_t nbNodes = 0;
        for (auto& pair : map) {
            nbNodes += 2 * pair.second.size();
        }
        nodes.reserve(nbNodes);
    }
    std::cout << "map number : " << map.size() << std::endl;
    for (auto& pair : map) {
        mergeFamilies(pair.second, intersectOnSinglePiece, polyConvexList, nodes);
    }
    // compute adjacence
    nodeAdjacence(nodes, polyConvexList);
    return nodes;
}

std::vector<Node> PathRendering::processFamilies(std::unordered_map<size_t, std::size_t>& families, const std::vector<PolyConvex>& polyConvexList,
                                                 std::vector<Family>& familyVector, std::vector<Intersection>& intersections,
                                                 std::vector<BoxIntersection>& box_intersection, CGAL::Union_find<std::size_t>& uf) {
    EASY_FUNCTION();

    std::unordered_set<Intersection> intersectOnSinglePiece;

    for (auto& pair : families) {
        std::size_t familyIndex = pair.second;
        Family& family = familyVector.at(familyIndex);
        family.createPatch(polyConvexList);
        if (family._patches.size() == 1) {
            intersectOnSinglePiece.insert(family._intersections.begin(), family._intersections.end());
        }
    }

    std::unordered_map<std::size_t, std::vector<const Family*>> map =
        locateFamilies(families, familyVector, intersections, box_intersection, uf, polyConvexList);

    // we avoid re allocation to preserve pointer

    std::vector<Node> nodes = createNode(map, intersectOnSinglePiece, polyConvexList);
    return nodes;
}

void PathRendering::createPolygonSet(const std::vector<PolyConvex>& polyConvexList, const std::vector<std::size_t>& cover,
                                     basic::Polygon_set_2Node& setPolygons) {

    std::vector<basic::Polygon_2Node> pnVect;
    pnVect.reserve(cover.size());
    for (const std::size_t i : cover) {
        const PolyConvex& pc = polyConvexList.at(i);
        pnVect.emplace_back(pc._geometry.vertices_begin(), pc._geometry.vertices_end());
    }
    // bug in CGAL, cannot use the aggregate version
    //   try {
    //    basic::Polygon_set_2Node bpolySet;
    //        bpolySet.join(pnVect.begin(), pnVect.end());
    //        setPolygons = bpolySet;
    //    } catch (CGAL::Failure_exception& e) {
    //        std::cout << e.message() << " " << std::string(e.what()) << std::endl;

    for (basic::Polygon_2Node& polyBasi : pnVect) {
        setPolygons.join(polyBasi);
    }

    //    }
}

void PathRendering::reCutAllGeometry(const std::vector<const Family*>& families, const std::vector<PolyConvex>& polyConvexList,
                                     const std::unordered_map<std::size_t, Node*>& map) {
    EASY_FUNCTION();
    std::cout << "reCutAllGeometry" << std::endl;

    std::vector<basic::Polygon_set_2Node> list_intersect_geometry;
    list_intersect_geometry.reserve(families.size());
    for (const Family* f : families) {
        if (f->_patches.size() > 1) {
            list_intersect_geometry.emplace_back();
            basic::Polygon_set_2Node& setPolygons = list_intersect_geometry.back();
            auto ite = f->_patches.begin();
            createPolygonSet(polyConvexList, ite->second, setPolygons);
            basic::Polygon_set_2Node setPolygons2;
            ++ite;
            createPolygonSet(polyConvexList, ite->second, setPolygons2);
            setPolygons.intersection(setPolygons2);
        }
    }
    basic::Polygon_set_2Node unionFamilies;
    for (basic::Polygon_set_2Node& setPolygons : list_intersect_geometry) {
        unionFamilies.join(setPolygons);
    }
    for (auto& p : map) {
        Node& n = *p.second;
        n._setPolygons.intersection(unionFamilies);
    }
}

void PathRendering::mergeFamilies(const std::vector<const Family*>& families, std::unordered_set<Intersection>& intersectOnSinglePiece,
                                  const std::vector<PolyConvex>& polyConvexList, std::vector<Node>& nodes) {
    EASY_FUNCTION();

    int32_t nodeId = _nextNodeId;

    std::unordered_set<std::size_t> coverSet;
    for (const Family* f : families) {
        if (f->_patches.size() > 1) {
            for (auto& pair : f->_patches) {

                coverSet.insert(pair.second.begin(), pair.second.end());
            }
        }
    }
    for (const Family* f : families) {
        if (f->_patches.size() == 1) {

            for (const Intersection& interval : f->_intersections) {
                if (coverSet.count(interval.first()) > 0) {
                    coverSet.emplace(interval.second());
                }
                else if (coverSet.count(interval.second()) > 0) {
                    coverSet.emplace(interval.first());
                }
            }
        }
    }
    CGAL::Union_find<std::size_t> uf;
    Family::createUnionFind(coverSet, polyConvexList, uf);

    std::vector<std::size_t> heads(coverSet.begin(), coverSet.end());

    for (std::size_t i = 0; i < heads.size(); ++i) {
        std::size_t first = heads.at(i);
        for (std::size_t j = i + 1; j < heads.size(); ++j) {
            std::size_t second = heads.at(j);
            const PolyConvex& p1 = polyConvexList.at(first);
            const PolyConvex& p2 = polyConvexList.at(second);
            if (!uf.same_set(p1.handle, p2.handle)) {
                if (intersectOnSinglePiece.count({first, second}) > 0) {
                    uf.unify_sets(p1.handle, p2.handle);
                }
            }
        }
    }

    if (uf.number_of_sets() > 1) {

        std::unordered_map<std::size_t, Node*> map;
        for (const std::size_t& i : coverSet) {

            const std::size_t head = *uf.find(polyConvexList.at(i).handle);

            auto ite = map.try_emplace(head, nullptr);
            Node*& np = ite.first->second;
            if (np == nullptr) {
                nodes.emplace_back(nodeId);
                ++nodeId;
                np = &nodes.back();
            }
            Node& n = *np;
            n._cover.emplace_back(i);
        }
        for (auto& p : map) {
            Node& n = *p.second;

            createPolygonSet(polyConvexList, n._cover, n._setPolygons);
            // Edge data annotation is no longer stored on curves in CGAL 5.x;
            // polygon membership is tracked via face data instead.

            for (auto& q : map) {
                if (p.first != q.first) {
                    n._opposite.push_back(q.second);
                }
            }
        }

        if (uf.number_of_sets() > 2) {
            reCutAllGeometry(families, polyConvexList, map);
        }
    }
    _nextNodeId = nodeId;
}
void PathRendering::pathRender(const std::vector<PolyConvex>& polyConvexList, OrientedRibbon& oRibbon) {
    PathRendering pathRender;
    std::cout << "polyConvexList.size() " << polyConvexList.size() << std::endl;

    pathRender.createIntersect(oRibbon, polyConvexList);
    std::cout << "end createIntersect  " << std::endl;
    pathRender.createUnion(oRibbon, polyConvexList);

    std::cout << "end union /intersection " << std::endl;
}
} /* namespace laby */
