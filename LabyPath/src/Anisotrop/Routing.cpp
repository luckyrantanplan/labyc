/*
 * Routing.cpp
 *
 *  Created on: Mar 27, 2018
 *      Author: florian
 */

#include "Routing.h"

#include "Anisotrop/Net.h"
#include "Anisotrop/QueueCost.h"
#include "Anisotrop/QueueElement.h"
#include "Anisotrop/SpatialIndex.h"
#include "basic/EasyProfilerCompat.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

#include "../basic/LinearGradient.h"
#include "protoc/AllConfig.pb.h"

namespace laby::aniso {

namespace {

using PointSet = CGAL::Point_set_2<Kernel>;

auto toInt32ConfigValue(uint32_t value, const char* fieldName) -> int32_t {
    constexpr auto kMaxInt32Value = static_cast<uint32_t>(std::numeric_limits<int32_t>::max());
    if (value > kMaxInt32Value) {
        throw std::out_of_range(fieldName);
    }
    return static_cast<int32_t>(value);
}

struct QueueUpdateState {
    int32_t direction = 0;
    int32_t parentId = 0;
};

struct PointConnectivityStats {
    std::size_t pointCount = 0;
    std::size_t noPointCount = 0;
};

auto collectCongestedNetIds(const Vertex& vertex) -> std::unordered_set<int32_t> {
    std::unordered_set<int32_t> netIds;
    for (const Halfedge& halfedge : RangeHelper::make(vertex.incident_halfedges())) {
        if (halfedge.curve().data().congestion() != 0) {
            netIds.emplace(halfedge.curve().data().getNet());
        }
    }
    return netIds;
}

auto shouldSkipHalfedge(const QueueElement& queueElement, const Halfedge& halfedge) -> bool {
    return !queueElement.polyConvex().empty() &&
           &halfedge.curve() == &queueElement.polyConvex()._supportHe->curve();
}

auto buildQueuedCost(const QueueElement& queueElement) -> QueueCost {
    QueueCost cost = queueElement.cost();
    cost.memorySource() = queueElement.cost().futureMemorySource();
    cost.futureMemorySource().clear();
    cost.memoryTarget() = queueElement.cost().futureMemoryTarget();
    cost.futureMemoryTarget().clear();
    return cost;
}

void applyDegreeCost(QueueCost& cost, std::size_t degree, const proto::RoutingCost& config) {
    if (degree > 2U) {
        cost.distance() += toInt32ConfigValue(config.distance_unit_cost(), "distance_unit_cost");
    }
}

void applyTargetMemoryPenalty(QueueCost& cost) {
    for (const int32_t targetNetId : cost.memoryTarget()) {
        if (cost.futureMemoryTarget().count(targetNetId) == 0U) {
            ++cost.congestion();
        }
    }
}

auto needsViaCost(const QueueElement& queueElement, int32_t direction) -> bool {
    return queueElement.direction() != -1 && direction != queueElement.direction();
}

auto shouldRelaxQueueElement(const QueueCost& cost, const QueueElement& queueElement) -> bool {
    return cost.congestion() == 0 &&
           (queueElement.cost().distance() == -1 || queueElement.cost() > cost);
}

void updateQueueElement(QueueElement& queueElement, const QueueCost& cost, QueueUpdateState state,
                        const PolyConvex& polyConvex, PriorityQueue& queue) {
    queueElement.cost() = cost;
    queueElement.setDirection(state.direction);
    queueElement.setParent(state.parentId);
    queueElement.polyConvex() = polyConvex;
    if (queueElement.isInQueue()) {
        queue.update(queueElement.handle());
        return;
    }
    queueElement.pushIn(queue);
}

void initializeMazeUnionFind(std::vector<PolyConvex>& polyConvexList,
                             CGAL::Union_find<std::size_t>& unionFind) {
    for (const PolyConvex& polyConvex : polyConvexList) {
        polyConvex.handle = unionFind.push_back(polyConvex._id);
    }

    for (const PolyConvex& polyConvex : polyConvexList) {
        for (std::size_t const adjacentId : polyConvex._adjacents) {
            unionFind.unify_sets(polyConvex.handle, polyConvexList.at(adjacentId).handle);
        }
    }
}

void buildMazePointConnectivity(
    const std::vector<PolyConvex>& polyConvexList, PointSet& pointSet,
    std::unordered_map<const PointSet::Vertex*, std::vector<std::size_t> /*unused*/>&
        vertexPolyConvexMap,
    std::vector<const PointSet::Vertex*>& orderedVertices, PointConnectivityStats& stats) {
    for (const PolyConvex& polyConvex : polyConvexList) {
        if (!polyConvex.hasPoints()) {
            ++stats.noPointCount;
            continue;
        }

        ++stats.pointCount;
        const Point_2& sourcePoint = polyConvex.getSourcePoint();
        const Point_2& targetPoint = polyConvex.getTargetPoint();

        const PointSet::Vertex* sourceVertex = &*pointSet.insert(sourcePoint);
        const PointSet::Vertex* targetVertex = &*pointSet.insert(targetPoint);

        auto sourceEntry = vertexPolyConvexMap.try_emplace(sourceVertex);
        sourceEntry.first->second.emplace_back(polyConvex._id);
        if (sourceEntry.second) {
            orderedVertices.emplace_back(sourceVertex);
        }

        auto targetEntry = vertexPolyConvexMap.try_emplace(targetVertex);
        targetEntry.first->second.emplace_back(polyConvex._id);
        if (targetEntry.second) {
            orderedVertices.emplace_back(targetVertex);
        }
    }
}

void connectSharedPointPolyConvexes(
    std::vector<PolyConvex>& polyConvexList, CGAL::Union_find<std::size_t>& unionFind,
    std::unordered_map<const PointSet::Vertex*, std::vector<std::size_t> /*unused*/>&
        vertexPolyConvexMap,
    const std::vector<const PointSet::Vertex*>& orderedVertices) {
    for (const PointSet::Vertex* vertex : orderedVertices) {
        std::vector<std::size_t>& connectedPolyConvexes = vertexPolyConvexMap.at(vertex);
        if (connectedPolyConvexes.empty()) {
            continue;
        }

        const CGAL::Union_find<std::size_t>::handle& firstHandle =
            polyConvexList.at(connectedPolyConvexes.at(0)).handle;
        for (std::size_t index = 1; index < connectedPolyConvexes.size(); ++index) {
            const CGAL::Union_find<std::size_t>::handle& currentHandle =
                polyConvexList.at(connectedPolyConvexes.at(index)).handle;
            if (unionFind.same_set(firstHandle, currentHandle)) {
                continue;
            }

            std::cout << "PolyConvex::connect(vec.at(0), vec.at(i) " << connectedPolyConvexes.at(0)
                      << " " << connectedPolyConvexes.at(index) << '\n';
            PolyConvex::connect(connectedPolyConvexes.at(0), connectedPolyConvexes.at(index),
                                polyConvexList);
            unionFind.unify_sets(firstHandle, currentHandle);
        }
    }
}

} // namespace

Routing::Routing(Arrangement_2& arr, proto::RoutingCost config)
    : _config(std::move(config)), //
      _arr(&arr),                 //
      _spatialIndex(_convexList), //
      _random{0, toInt32ConfigValue(_config.max_random(), "max_random"), _config.seed()} {
    EASY_FUNCTION();
    _convexList.reserve(_arr->number_of_edges());

    _edgesQList.reserve(_arr->number_of_edges());
    for (Vertex& vertex : RangeHelper::make(_arr->vertices_begin(), _arr->vertices_end())) {
        vertex.data().setId(static_cast<int32_t>(_edgesQList.size()));
        _edgesQList.emplace_back(vertex);
    }
}

static void Routing::connectTwoPinPath(const std::vector<aniso::Net>& nets,
                                       const SpatialIndex& spatialIndex,
                                       std::vector<PolyConvex>& convexList) {
    std::unordered_map<const Vertex*, std::vector<std::size_t>> mapVertexPolyConvex;
    std::vector<const Vertex*> orderedList; // predictable order

    for (const aniso::Net& net : nets) {
        if (net.isPlaced()) {

            {
                const aniso::Pin& pin1 = net.source();
                auto ite = mapVertexPolyConvex.try_emplace(&pin1.vertex());
                ite.first->second.emplace_back(pin1.polyConvexIndex());
                if (ite.second) { // actually inserted
                    orderedList.emplace_back(&pin1.vertex());
                }
            }
            {
                const aniso::Pin& pin2 = net.target();
                auto ite = mapVertexPolyConvex.try_emplace(&pin2.vertex());

                ite.first->second.emplace_back(pin2.polyConvexIndex());
                if (ite.second) { // actually inserted
                    orderedList.emplace_back(&pin2.vertex());
                }
            }
        }
    }
    for (const Vertex* vertexP : orderedList) {

        const std::vector<std::size_t>& list = mapVertexPolyConvex.at(vertexP);
        uint32_t index = 0;

        if (list.size() == 1 and
            spatialIndex.isPointInside(convexList.at(list.at(0)), *vertexP, index)) {
            std::cout << "orphan PolyConvex connect not empty  list.at(0) index" << list.at(0)
                      << " " << index << '\n';
            PolyConvex::connect(list.at(0), index, convexList);
        }
        for (std::size_t i = 0; i < list.size(); ++i) {
            for (std::size_t j = i + 1; j < list.size(); ++j) {

                PolyConvex::connect(list.at(i), list.at(j), convexList, vertexP->point());
            }
        }
    }
}

static void Routing::connectMaze(std::vector<PolyConvex>& polyConvexList) {

    // init structures

    std::cout << " connectMaze \n";

    std::vector<std::size_t> const highDegreePolyConvex;
    std::unordered_set<basic::PairInteger> const adjacenceList;

    CGAL::Union_find<std::size_t> unionFind;

    initializeMazeUnionFind(polyConvexList, unionFind);

    // make a Point Set
    PointSet pointSet;
    // with a map Vertex* -> list of PolyConvex
    std::unordered_map<const PointSet::Vertex*, std::vector<std::size_t>> map;
    std::vector<const PointSet::Vertex*> orderedVertex;
    PointConnectivityStats const connectivityStats;
    buildMazePointConnectivity(polyConvexList, pointSet, map, orderedVertex, connectivityStats);
    std::cout << "statPoin " << connectivityStats.pointCount << " statNoPoint "
              << connectivityStats.noPointCount << '\n';

    // for each element on map, loop on polyconvex list, and connect if distinct set

    std::cout << " connectMaze  1st connection begin\n";

    connectSharedPointPolyConvexes(polyConvexList, unionFind, map, orderedVertex);
    std::cout << " connectMaze  1st connection end\n";
    // walk on all pair of point

    std::vector<SegmentPS> vectSeg;

    for (auto& edge :
         RangeHelper::make(pointSet.finite_edges_begin(), pointSet.finite_edges_end())) {
        const PointSet::Face_handle& faceHandle = edge.first;
        const int edgeIndex = edge.second;
        const SegmentPS seg(&*faceHandle->vertex(PointSet::cw(edgeIndex)),
                            &*faceHandle->vertex(PointSet::ccw(edgeIndex)));

        if (!unionFind.same_set(polyConvexList.at(map.at(seg.source()).front()).handle,
                                polyConvexList.at(map.at(seg.target()).front()).handle)) {

            vectSeg.emplace_back(seg);
        }
    }
    std::sort(vectSeg.begin(), vectSeg.end());

    const double thickness = 2.;

    for (const SegmentPS& seg : vectSeg) {
        const std::size_t pcId1 = map.at(seg.source()).front();
        const std::size_t pcId2 = map.at(seg.target()).front();
        const CGAL::Union_find<std::size_t>::handle& handle1 = polyConvexList.at(pcId1).handle;
        const CGAL::Union_find<std::size_t>::handle& handle2 = polyConvexList.at(pcId2).handle;
        if (!unionFind.same_set(handle1, handle2)) {

            // create a PolyConvex

            basic::LinearGradient lgrad(seg.source()->point(), thickness, seg.target()->point(),
                                        thickness);

            polyConvexList.emplace_back(seg.source()->point(), seg.target()->point(),
                                        polyConvexList.size(), lgrad);
            // warning : references are undefined now ( we have increase the size of the vector)

            const PolyConvex& newPoly = polyConvexList.back();

            PolyConvex::connect(newPoly._id, pcId1, polyConvexList, seg.source()->point());
            PolyConvex::connect(newPoly._id, pcId2, polyConvexList, seg.target()->point());
            // connect with others

            unionFind.unify_sets(polyConvexList.at(pcId1).handle, polyConvexList.at(pcId2).handle);

            std::cout << " PolyConvex::connect( " << newPoly._id << " " << pcId1 << " " << pcId2
                      << '\n';
        }
    }
    std::cout << " connectMaze  2nd  connection completed\n";
    // end : reset handle just in case

    for (const PolyConvex& polyConvex : polyConvexList) {
        polyConvex.resetMutable();
    }
}

// experimental : need to work on placement of cut
void Routing::createMaze() {

    // init structures

    std::vector<std::size_t> highDegreePolyConvex;
    std::unordered_set<basic::PairInteger> adjacenceList;

    CGAL::Union_find<std::size_t> unionFind;
    for (const PolyConvex& polyConvex : _convexList) {
        if (polyConvex._adjacents.size() > 2UL) {
            highDegreePolyConvex.emplace_back(polyConvex._id);
        }
        for (std::size_t const adjacency : polyConvex._adjacents) {
            adjacenceList.emplace(polyConvex._id, adjacency);
        }

        polyConvex.handle = unionFind.push_back(polyConvex._id);
    }

    // create roots

    // this loop could be optimize inside the init loop (to avoid erasing adjacence_list elements)
    for (std::size_t const polyConvexId : highDegreePolyConvex) {
        PolyConvex const& polyConvex = _convexList.at(polyConvexId);
        for (std::size_t const adjacency : polyConvex._adjacents) {
            auto ite = adjacenceList.find(basic::PairInteger{polyConvex._id, adjacency});
            if (ite != adjacenceList.end()) {
                unionFind.unify_sets(polyConvex.handle, _convexList.at(adjacency).handle);
                adjacenceList.erase(ite);
            }
        }
    }

    // should randomize access
    for (const basic::PairInteger& pair : adjacenceList) {
        PolyConvex& pc1 = _convexList.at(pair.first());
        PolyConvex& pc2 = _convexList.at(pair.second());
        const CGAL::Union_find<std::size_t>::handle& handle1 = pc1.handle;
        const CGAL::Union_find<std::size_t>::handle& handle2 = pc2.handle;
        if (unionFind.same_set(handle1, handle2)) {
            std::cout << "could cut between " << pair.first() << " and " << pair.second() << '\n';
            pc1.removeAdjacence(pc2._id);
            pc2.removeAdjacence(pc1._id);

            pc2._geometry.clear();
        } else {
            unionFind.unify_sets(handle1, handle2);
        }
    }

    // end : reset handle just in case

    for (const PolyConvex& polyConvex : _convexList) {
        polyConvex.resetMutable();
    }
}

void Routing::commitNewPath(const int32_t& targetId, Net& net) {
    Pin const& pin1 = net.source();
    Pin& pin2 = net.target();
    const std::size_t begin = 0 = _convexList.size();

    for (int32_t const id = targetId; id != -1;
         id = _edgesQList.at(static_cast<std::size_t>(id)).parent()) {

        PolyConvex& polyConvex = _edgesQList.at(static_cast<std::size_t>(id)).polyConvex();
        if (!polyConvex.empty()) {
            polyConvex._id = _convexList.size();
            _convexList.emplace_back(polyConvex);
            net.path().emplace_back(polyConvex._id);
        }
    }

    PolyConvex::connect(begin, _convexList);
    pin1.setPolyConvexIndex(_convexList.size() - 1);

    pin2.setPolyConvexIndex(begin);
    for (std::size_t j = begin; j < _convexList.size(); ++j) {
        _convexList.at(j)._supportHe->curve().data().addPath(net.id());
        _spatialIndex.insert(_convexList.at(j));
    }
    net.markPlaced();
}

auto Routing::findRoute(Net& net) -> bool {
    EASY_FUNCTION();

    for (QueueElement& queueElement : _edgesQList) {
        queueElement.clear();
    }

    PriorityQueue queue;

    const Pin& pin1 = net.source();
    const Pin& pin2 = net.target();
    const int32_t& sourceId = pin1.vertex().data().id();

    QueueElement& sourceQueueElement = _edgesQList.at(static_cast<std::size_t>(sourceId));
    const int32_t& targetId = pin2.vertex().data().id();
    basic::LinearGradient const linearGradient = net.gradient();

    sourceQueueElement.pushIn(queue);
    sourceQueueElement.cost().distance() = 0;
    sourceQueueElement.cost().viaNum() = 0;
    sourceQueueElement.cost().memorySource() = collectCongestedNetIds(pin1.vertex());

    std::unordered_set<int32_t> const targetNets = collectCongestedNetIds(pin2.vertex());

    int32_t const priorityNumber = 0;
    bool solved = false;
    while (!queue.empty()) {
        // Safe: the pairing heap stores QueueElement* pointers into _edgesQList,
        // so queue.pop() removes the pointer from the heap but the QueueElement
        // object in _edgesQList remains valid for the rest of the loop body.
        QueueElement& topQueueElement = *queue.top();

        Vertex& vertex = topQueueElement.vertex();
        if (vertex.data().id() == targetId) {
            std::cout << "END !!!!" << topQueueElement.cost() << "\n";
            solved = true;
            break;
        }
        queue.pop();
        topQueueElement.resetHandle();

        const std::size_t degree = vertex.degree();

        for (Halfedge& halfedge : RangeHelper::make(vertex.incident_halfedges())) {
            if (shouldSkipHalfedge(topQueueElement, halfedge)) {
                continue;
            }

            QueueCost cost = buildQueuedCost(topQueueElement);
            applyDegreeCost(cost, degree, _config);

            const int32_t newDirection = halfedge.curve().data().direction();
            const int32_t congestion = halfedge.curve().data().congestion();
            cost.congestion() += congestion;

            const PolyConvex candidatePolyConvex(halfedge, 0, linearGradient);

            if (congestion == 0) {
                _spatialIndex.updateCostIfIntersect(candidatePolyConvex, net, targetNets, cost);
            }
            if (cost.congestion() == 0) {
                applyTargetMemoryPenalty(cost);
            }
            if (needsViaCost(topQueueElement, newDirection)) {
                cost.distance() += toInt32ConfigValue(_config.via_unit_cost(), "via_unit_cost");
            }

            const int32_t newId = halfedge.source()->data().id(); // vertex id
            QueueElement& nextQueueElement = _edgesQList.at(static_cast<std::size_t>(newId));
            if (!shouldRelaxQueueElement(cost, nextQueueElement)) {
                continue;
            }

            cost.randomization() = _random.get();
            --priorityNumber;
            updateQueueElement(nextQueueElement, cost,
                               QueueUpdateState{newDirection, topQueueElement.vertex().data().id()},
                               candidatePolyConvex, queue);
        }
    }

    if (solved) {
        commitNewPath(targetId, net);
    }

    return solved;
}

} // namespace laby::aniso
