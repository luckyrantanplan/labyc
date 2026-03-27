/*
 * PathRendering.cpp
 *
 *  Created on: Mar 8, 2018
 *      Author: florian
 */

#include "PathRendering.h"

#include <CGAL/Arrangement_on_surface_2.h>
#include <CGAL/Bbox_2.h>
#include <CGAL/Boolean_set_operations_2/Gps_on_surface_base_2.h>
#include <CGAL/box_intersection_d.h>
#include <cstdint>

#include "basic/EasyProfilerCompat.h"
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_set_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <queue>
#include <utility>

#include "../OrientedRibbon.h"
#include "../basic/PolygonTools.h"
#include "NodeRendering.h"

namespace laby {

using BoxPolyConvex = CGAL::Box_intersection_d::Box_with_handle_d<double, 2, const PolyConvex*>;

namespace {

constexpr int32_t kUnassignedState = -1;
constexpr int32_t kPrimaryAdjacentState = 0;
constexpr int32_t kSecondaryAdjacentState = 1;

auto enqueueUnvisitedAdjacentPolyConvexes(Node& node, const PolyConvex& polyConvex,
                                          const std::vector<PolyConvex>& polyConvexList,
                                          std::queue<QueueNodePolyConvex>& queue) -> void {
    for (const std::size_t adjacentIndex : polyConvex._adjacents) {
        if (polyConvexList.at(adjacentIndex)._visited != -1) {
            queue.emplace(node, polyConvexList.at(adjacentIndex));
        }
    }
}

auto connectPolyConvexNodes(const PolyConvex& polyConvex) -> void {
    std::vector<Node*>& polyConvexNodes = polyConvex._nodes;
    while (polyConvexNodes.size() > 1) {
        Node& node = *polyConvexNodes.back();
        polyConvexNodes.resize(polyConvexNodes.size() - 1U);
        for (Node* adjacentNode : polyConvexNodes) {
            node._adjacents.emplace(adjacentNode);
            adjacentNode->_adjacents.emplace(&node);
        }
    }
}

auto seedNodeAdjacenceQueue(Node& node, const std::vector<PolyConvex>& polyConvexList,
                            std::queue<QueueNodePolyConvex>& queue) -> void {
    for (const std::size_t coverIndex : node._cover) {
        const PolyConvex& polyConvex = polyConvexList.at(coverIndex);
        if (polyConvex._visited != -1) {
            queue.emplace(node, polyConvex);
            break;
        }
    }
}

auto processQueuedPolyConvex(QueueNodePolyConvex queuedPolyConvex,
                             const std::vector<PolyConvex>& polyConvexList,
                             std::queue<QueueNodePolyConvex>& queue) -> void {
    queuedPolyConvex.polyConvex()._visited = -1;
    const PolyConvex& polyConvex = queuedPolyConvex.polyConvex();
    if (polyConvex._nodes.empty() || polyConvex._nodes.front() == &queuedPolyConvex.node()) {
        enqueueUnvisitedAdjacentPolyConvexes(queuedPolyConvex.node(), polyConvex, polyConvexList,
                                             queue);
        return;
    }

    Node& adjacentNode = *polyConvex._nodes.front();
    adjacentNode._adjacents.emplace(&queuedPolyConvex.node());
    queuedPolyConvex.node()._adjacents.emplace(&adjacentNode);
    enqueueUnvisitedAdjacentPolyConvexes(adjacentNode, polyConvex, polyConvexList, queue);
}

auto initializeBeginNodeState(Node& beginNode) -> void {
    StateSelect stateSelect(beginNode._opposite);
    beginNode.setState(stateSelect.getNext());
}

auto enqueueOppositeNodes(Node& node, std::priority_queue<NodeQueue>& queue) -> void {
    StateSelect stateSelect(node._opposite);
    stateSelect.markOccupied(node._state);

    for (Node* oppositeNode : node._opposite) {
        int32_t& oppositeState = oppositeNode->_state;
        if (oppositeState == kUnassignedState) {
            oppositeState = stateSelect.getNext();
            queue.emplace(*oppositeNode);
        }
    }
}

auto enqueueAdjacentNodes(Node& node, std::priority_queue<NodeQueue>& queue) -> void {
    const int32_t adjacentState =
        (node._state == kPrimaryAdjacentState) ? kSecondaryAdjacentState : kPrimaryAdjacentState;
    for (Node* adjacentNode : node._adjacents) {
        int32_t& nodeState = adjacentNode->_state;
        if (nodeState == kUnassignedState && !adjacentNode->haveOppositeState()) {
            nodeState = adjacentState;
            queue.emplace(*adjacentNode);
        }
    }
}

auto collectMultiPatchCoverSet(const std::vector<const Family*>& families)
    -> std::unordered_set<std::size_t> {
    std::unordered_set<std::size_t> coverSet;
    for (const Family* family : families) {
        if (family->_patches.size() > 1) {
            for (const auto& patchEntry : family->_patches) {
                coverSet.insert(patchEntry.second.begin(), patchEntry.second.end());
            }
        }
    }
    return coverSet;
}

auto extendCoverSetFromSinglePatchFamilies(const std::vector<const Family*>& families,
                                           std::unordered_set<std::size_t>& coverSet) -> void {
    for (const Family* family : families) {
        if (family->_patches.size() == 1) {
            for (const Intersection& interval : family->_intersections) {
                if (coverSet.count(interval.first()) > 0) {
                    coverSet.emplace(interval.second());
                } else if (coverSet.count(interval.second()) > 0) {
                    coverSet.emplace(interval.first());
                }
            }
        }
    }
}

auto unifyIntersectingCoverSet(const std::vector<std::size_t>& heads,
                               const std::vector<PolyConvex>& polyConvexList,
                               const std::unordered_set<Intersection>& intersectOnSinglePiece,
                               CGAL::Union_find<std::size_t>& unionFind) -> void {
    for (std::size_t firstIndex = 0; firstIndex < heads.size(); ++firstIndex) {
        const std::size_t first = heads.at(firstIndex);
        for (std::size_t secondIndex = firstIndex + 1; secondIndex < heads.size(); ++secondIndex) {
            const std::size_t second = heads.at(secondIndex);
            const PolyConvex& firstPolyConvex = polyConvexList.at(first);
            const PolyConvex& secondPolyConvex = polyConvexList.at(second);
            if (!unionFind.same_set(firstPolyConvex.handle, secondPolyConvex.handle) &&
                intersectOnSinglePiece.count({first, second}) > 0) {
                unionFind.unify_sets(firstPolyConvex.handle, secondPolyConvex.handle);
            }
        }
    }
}

auto buildNodeMapFromCoverSet(const std::unordered_set<std::size_t>& coverSet,
                              const std::vector<PolyConvex>& polyConvexList,
                              CGAL::Union_find<std::size_t>& unionFind, std::vector<Node>& nodes,
                              int32_t& nodeId) -> std::unordered_map<std::size_t, Node*> {
    std::unordered_map<std::size_t, Node*> nodeMap;
    for (const std::size_t& coverIndex : coverSet) {
        const std::size_t head = *unionFind.find(polyConvexList.at(coverIndex).handle);

        auto iterator = nodeMap.try_emplace(head, nullptr);
        Node*& nodePointer = iterator.first->second;
        if (nodePointer == nullptr) {
            nodes.emplace_back(nodeId);
            ++nodeId;
            nodePointer = &nodes.back();
        }
        nodePointer->_cover.emplace_back(coverIndex);
    }
    return nodeMap;
}

} // namespace

void PathRendering::unify(std::size_t secondIndex, const std::vector<std::size_t>& adjacentIndices,
                          std::unordered_set<Intersection>& intersections,
                          CGAL::Union_find<size_t>& unionFind, const Intersection& intersection) {

    EASY_FUNCTION();
    for (std::size_t adjacentIndex : adjacentIndices) {
        const auto iterator = intersections.find({adjacentIndex, secondIndex});
        if (iterator != intersections.end()) {
            unionFind.unify_sets(intersection.handle(), iterator->handle());
        }
    }
}

void PathRendering::createUnion(OrientedRibbon& ribs,
                                const std::vector<PolyConvex>& polyConvexList) {
    EASY_FUNCTION();
    CGAL::Polygon_set_2<Kernel> set;

    std::vector<Linear_polygon> polygonList;
    polygonList.reserve(polyConvexList.size());
    for (const PolyConvex& polyConvex : polyConvexList) {
        polygonList.emplace_back(polyConvex._geometry);
    }
    set.join(polygonList.begin(), polygonList.end());

    std::vector<CGAL::Polygon_with_holes_2<Kernel>> polygons;

    set.polygons_with_holes(std::back_inserter(polygons));

    for (const CGAL::Polygon_with_holes_2<Kernel>& polygon : polygons) {
        for (const Linear_polygon::Segment_2& seg : RangeHelper::make(
                 polygon.outer_boundary().edges_begin(), polygon.outer_boundary().edges_end())) {
            ribs.addCCW(seg);
        }

        for (const auto& hole : RangeHelper::make(polygon.holes_begin(), polygon.holes_end())) {
            for (const Linear_polygon::Segment_2& seg :
                 RangeHelper::make(hole.edges_begin(), hole.edges_end())) {
                ribs.addCW(seg);
            }
        }
    }
}

auto PathRendering::doIntersect(Intersection& firstIntersection, Intersection& secondIntersection,
                                const std::vector<PolyConvex>& polyConvexList) -> bool {
    EASY_FUNCTION();
    const Linear_polygon& secondLeftPolygon =
        polyConvexList.at(secondIntersection.first())._geometry;
    const Linear_polygon& secondRightPolygon =
        polyConvexList.at(secondIntersection.second())._geometry;
    const Linear_polygon& firstLeftPolygon = polyConvexList.at(firstIntersection.first())._geometry;
    const Linear_polygon& firstRightPolygon =
        polyConvexList.at(firstIntersection.second())._geometry;

    if (PolyConvex::testConvexPolyIntersect(firstLeftPolygon, secondLeftPolygon)) {
        if (PolyConvex::testConvexPolyIntersect(firstLeftPolygon, secondRightPolygon)) {
            if (PolyConvex::testConvexPolyIntersect(firstRightPolygon, secondLeftPolygon)) {
                if (PolyConvex::testConvexPolyIntersect(firstRightPolygon, secondRightPolygon)) {

                    CGAL::Polygon_set_2<Kernel> set(secondRightPolygon);
                    set.intersection(firstLeftPolygon);
                    set.intersection(firstRightPolygon);

                    std::vector<CGAL::Polygon_with_holes_2<Kernel>> intersectedPolygons;
                    set.polygons_with_holes(std::back_inserter(intersectedPolygons));

                    for (const CGAL::Polygon_with_holes_2<Kernel>& polygonWithHoles :
                         intersectedPolygons) {
                        const Linear_polygon& outerBoundary = polygonWithHoles.outer_boundary();

                        if (PolyConvex::testConvexPolyIntersect(outerBoundary, secondLeftPolygon)) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

auto PathRendering::locateFamilies(const std::unordered_map<std::size_t, std::size_t>& families,
                                   std::vector<Family>& familyVector,
                                   std::vector<Intersection>& intersections,
                                   std::vector<PathRendering::BoxIntersection>& boxIntersectionList,
                                   const CGAL::Union_find<std::size_t>& unionFind,
                                   const std::vector<PolyConvex>& polyConvexList)
    -> std::unordered_map<std::size_t, std::vector<const Family*>> {

    EASY_FUNCTION();
    CGAL::Union_find<std::size_t> familyUnionFind;

    for (const auto& familyEntry : families) {
        std::size_t intersectionIndex = familyEntry.first;
        intersections.at(intersectionIndex)
            .setFamilyHandle(familyUnionFind.push_back(intersectionIndex));
    }
    CGAL::box_self_intersection_d(
        boxIntersectionList.begin(), boxIntersectionList.end(),
        [&](const BoxIntersection& firstBoxIntersection,
            const BoxIntersection& secondBoxIntersection) {
            Intersection firstIntersection = intersections.at(firstBoxIntersection.handle());
            Intersection secondIntersection = intersections.at(secondBoxIntersection.handle());

            const std::size_t firstFamilyHead = *unionFind.find(firstIntersection.handle());
            const std::size_t secondFamilyHead = *unionFind.find(secondIntersection.handle());
            if (firstFamilyHead != secondFamilyHead) {
                Intersection firstHeadIntersection = intersections.at(firstFamilyHead);
                Intersection secondHeadIntersection = intersections.at(secondFamilyHead);
                if (!familyUnionFind.same_set(firstHeadIntersection.familyHandle(),
                                              secondHeadIntersection.familyHandle())) {
                    bool intersects =
                        doIntersect(firstIntersection, secondIntersection, polyConvexList);
                    if (intersects) {
                        familyUnionFind.unify_sets(firstHeadIntersection.familyHandle(),
                                                   secondHeadIntersection.familyHandle());
                    }
                }
            }
        });
    std::unordered_map<std::size_t, std::vector<const Family*>> groupedFamilies;

    for (const auto& familyEntry : families) {
        std::size_t intersectionIndex = familyEntry.first;
        std::size_t head =
            *familyUnionFind.find(intersections.at(intersectionIndex).familyHandle());
        auto iterator = groupedFamilies.try_emplace(head);
        iterator.first->second.emplace_back(&familyVector.at(familyEntry.second));
    }

    return groupedFamilies;
}

void PathRendering::createIntersect(OrientedRibbon& oribbon,
                                    const std::vector<PolyConvex>& polyConvexList) {
    EASY_FUNCTION();
    std::cout << "start intersect\n";

    std::vector<Intersection> intersections;
    intersections.reserve(polyConvexList.size());
    std::vector<BoxPolyConvex> boxes;
    boxes.reserve(polyConvexList.size());
    for (const PolyConvex& polyConvex : polyConvexList) {
        boxes.emplace_back(polyConvex._geometry.bbox(), &polyConvex);
    }

    std::cout << "start boxes  boxes size " << boxes.size() << '\n';

    std::vector<BoxIntersection> boxIntersectionList;
    boxIntersectionList.reserve(boxes.size());

    CGAL::box_self_intersection_d(
        boxes.begin(), boxes.end(),
        [&](const BoxPolyConvex& firstBox, const BoxPolyConvex& secondBox) {
            const std::vector<std::size_t>& adjacentIndices = firstBox.handle()->_adjacents;
            const auto iterator =
                std::find(adjacentIndices.begin(), adjacentIndices.end(), secondBox.handle()->_id);
            if (iterator != adjacentIndices.end()) {
                intersections.emplace_back(firstBox.handle()->_id, secondBox.handle()->_id);
            } else {

                if (PolyConvex::testConvexPolyIntersect(firstBox.handle()->_geometry,
                                                        secondBox.handle()->_geometry)) {
                    intersections.emplace_back(firstBox.handle()->_id, secondBox.handle()->_id);
                    CGAL::Bbox_2 firstBbox = firstBox.handle()->_geometry.bbox();
                    CGAL::Bbox_2 secondBbox = secondBox.handle()->_geometry.bbox();

                    const double xmin = std::max(firstBbox.xmin(), secondBbox.xmin());
                    const double xmax = std::min(firstBbox.xmax(), secondBbox.xmax());
                    const double ymin = std::max(firstBbox.ymin(), secondBbox.ymin());
                    const double ymax = std::min(firstBbox.ymax(), secondBbox.ymax());
                    CGAL::Bbox_2 intersectBox(xmin, ymin, xmax, ymax);

                    boxIntersectionList.emplace_back(intersectBox, intersections.size() - 1);
                }
            }
        });
    std::cout << "end boxes\n";
    CGAL::Union_find<std::size_t> unionFind;

    for (std::size_t intersectionIndex = 0; intersectionIndex < intersections.size();
         ++intersectionIndex) {
        intersections.at(intersectionIndex).setHandle(unionFind.push_back(intersectionIndex));
    }

    std::unordered_set<Intersection> intersectionsSet(intersections.begin(), intersections.end());
    for (const Intersection& intersection : intersections) {
        const std::vector<std::size_t>& firstAdjacents =
            polyConvexList.at(intersection.first())._adjacents;
        const std::vector<std::size_t>& secondAdjacents =
            polyConvexList.at(intersection.second())._adjacents;

        unify(intersection.second(), firstAdjacents, intersectionsSet, unionFind, intersection);
        unify(intersection.first(), secondAdjacents, intersectionsSet, unionFind, intersection);
    }

    std::cout << "unify\n";

    std::unordered_map<std::size_t, std::size_t> families;
    std::vector<Family> familyVector;
    familyVector.reserve(intersections.size());
    for (const Intersection& intersection : intersections) {

        std::size_t head = *unionFind.find(intersection.handle());
        auto iterator = families.try_emplace(head, familyVector.size());
        if (iterator.second) {
            familyVector.emplace_back();
        }
        familyVector.at(iterator.first->second)._intersections.emplace_back(intersection);
    }

    std::vector<Node> nodes = processFamilies(families, polyConvexList, familyVector, intersections,
                                              boxIntersectionList, unionFind);
    std::cout << "processFamilies\n";
    chooseNodeState(nodes);
    std::cout << "chooseNodeState\n";
    NodeRendering::render(oribbon, nodes, polyConvexList);
}

void PathRendering::nodeAdjacence(std::vector<Node>& nodes,
                                  const std::vector<PolyConvex>& polyConvexList) {
    EASY_FUNCTION();
    for (Node& node : nodes) {
        for (const std::size_t coverIndex : node._cover) {
            polyConvexList.at(coverIndex)._nodes.emplace_back(&node);
        }
    }

    for (const PolyConvex& polyConvex : polyConvexList) {
        connectPolyConvexNodes(polyConvex);
    }

    for (Node& node : nodes) {
        std::queue<QueueNodePolyConvex> queue;
        seedNodeAdjacenceQueue(node, polyConvexList, queue);
        while (!queue.empty()) {
            QueueNodePolyConvex queuedPolyConvex = queue.front();
            queue.pop();
            processQueuedPolyConvex(queuedPolyConvex, polyConvexList, queue);
        }
    }
}

void PathRendering::chooseNodeState(std::vector<Node>& nodes) {
    EASY_FUNCTION();
    for (Node& beginNode : nodes) {
        if (beginNode._state == kUnassignedState) {
            initializeBeginNodeState(beginNode);

            std::priority_queue<NodeQueue> queue;
            queue.emplace(beginNode);
            while (!queue.empty()) {
                Node& node = queue.top().node();
                queue.pop();
                enqueueOppositeNodes(node, queue);
                enqueueAdjacentNodes(node, queue);
            }
        }
    }
}

auto PathRendering::createNode(
    const std::unordered_map<std::size_t, std::vector<const Family*>>& familyMap,
    std::unordered_set<Intersection> intersectOnSinglePiece,
    const std::vector<PolyConvex>& polyConvexList) -> std::vector<Node> {
    EASY_FUNCTION();
    std::vector<Node> nodes;
    {
        std::size_t nbNodes = 0;
        for (const auto& familyEntry : familyMap) {
            nbNodes += 2 * familyEntry.second.size();
        }
        nodes.reserve(nbNodes);
    }
    std::cout << "map number : " << familyMap.size() << '\n';
    for (const auto& familyEntry : familyMap) {
        mergeFamilies(familyEntry.second, intersectOnSinglePiece, polyConvexList, nodes);
    }
    nodeAdjacence(nodes, polyConvexList);
    return nodes;
}

auto PathRendering::processFamilies(std::unordered_map<size_t, std::size_t>& families,
                                    const std::vector<PolyConvex>& polyConvexList,
                                    std::vector<Family>& familyVector,
                                    std::vector<Intersection>& intersections,
                                    std::vector<BoxIntersection>& box_intersection,
                                    CGAL::Union_find<std::size_t>& unionFind) -> std::vector<Node> {
    EASY_FUNCTION();

    std::unordered_set<Intersection> intersectOnSinglePiece;

    for (const auto& familyEntry : families) {
        std::size_t familyIndex = familyEntry.second;
        Family& family = familyVector.at(familyIndex);
        family.createPatch(polyConvexList);
        if (family._patches.size() == 1) {
            intersectOnSinglePiece.insert(family._intersections.begin(),
                                          family._intersections.end());
        }
    }

    std::unordered_map<std::size_t, std::vector<const Family*>> map = locateFamilies(
        families, familyVector, intersections, box_intersection, unionFind, polyConvexList);

    std::vector<Node> nodes = createNode(map, intersectOnSinglePiece, polyConvexList);
    return nodes;
}

void PathRendering::createPolygonSet(const std::vector<PolyConvex>& polyConvexList,
                                     const std::vector<std::size_t>& cover,
                                     basic::Polygon_set_2Node& setPolygons) {

    std::vector<basic::Polygon_2Node> polygonVector;
    polygonVector.reserve(cover.size());
    for (const std::size_t coverIndex : cover) {
        const PolyConvex& polyConvex = polyConvexList.at(coverIndex);
        polygonVector.emplace_back(polyConvex._geometry.vertices_begin(),
                                   polyConvex._geometry.vertices_end());
    }

    for (basic::Polygon_2Node& polygon : polygonVector) {
        setPolygons.join(polygon);
    }
}

void PathRendering::reCutAllGeometry(const std::vector<const Family*>& families,
                                     const std::vector<PolyConvex>& polyConvexList,
                                     const std::unordered_map<std::size_t, Node*>& map) {
    EASY_FUNCTION();
    std::cout << "reCutAllGeometry\n";

    std::vector<basic::Polygon_set_2Node> intersectGeometryList;
    intersectGeometryList.reserve(families.size());
    for (const Family* family : families) {
        if (family->_patches.size() > 1) {
            intersectGeometryList.emplace_back();
            basic::Polygon_set_2Node& setPolygons = intersectGeometryList.back();
            auto patchIterator = family->_patches.begin();
            createPolygonSet(polyConvexList, patchIterator->second, setPolygons);
            basic::Polygon_set_2Node secondaryPolygonSet;
            ++patchIterator;
            createPolygonSet(polyConvexList, patchIterator->second, secondaryPolygonSet);
            setPolygons.intersection(secondaryPolygonSet);
        }
    }
    basic::Polygon_set_2Node unionFamilies;
    for (basic::Polygon_set_2Node& setPolygons : intersectGeometryList) {
        unionFamilies.join(setPolygons);
    }
    for (const auto& nodeEntry : map) {
        Node& node = *nodeEntry.second;
        node._setPolygons.intersection(unionFamilies);
    }
}

void PathRendering::mergeFamilies(const std::vector<const Family*>& families,
                                  std::unordered_set<Intersection>& intersectOnSinglePiece,
                                  const std::vector<PolyConvex>& polyConvexList,
                                  std::vector<Node>& nodes) {
    EASY_FUNCTION();

    int32_t nodeId = _nextNodeId;

    std::unordered_set<std::size_t> coverSet = collectMultiPatchCoverSet(families);
    extendCoverSetFromSinglePatchFamilies(families, coverSet);

    CGAL::Union_find<std::size_t> unionFind;
    Family::createUnionFind(coverSet, polyConvexList, unionFind);

    std::vector<std::size_t> heads(coverSet.begin(), coverSet.end());

    unifyIntersectingCoverSet(heads, polyConvexList, intersectOnSinglePiece, unionFind);

    if (unionFind.number_of_sets() > 1) {
        std::unordered_map<std::size_t, Node*> map =
            buildNodeMapFromCoverSet(coverSet, polyConvexList, unionFind, nodes, nodeId);
        for (const auto& nodeEntry : map) {
            Node& node = *nodeEntry.second;

            createPolygonSet(polyConvexList, node._cover, node._setPolygons);

            for (const auto& oppositeEntry : map) {
                if (nodeEntry.first != oppositeEntry.first) {
                    node._opposite.push_back(oppositeEntry.second);
                }
            }
        }

        if (unionFind.number_of_sets() > 2) {
            reCutAllGeometry(families, polyConvexList, map);
        }
    }
    _nextNodeId = nodeId;
}
void PathRendering::pathRender(const std::vector<PolyConvex>& polyConvexList,
                               OrientedRibbon& oRibbon) {
    PathRendering pathRenderer;
    std::cout << "polyConvexList.size() " << polyConvexList.size() << '\n';

    pathRenderer.createIntersect(oRibbon, polyConvexList);
    std::cout << "end createIntersect  \n";
    PathRendering::createUnion(oRibbon, polyConvexList);

    std::cout << "end union /intersection \n";
}
} /* namespace laby */
