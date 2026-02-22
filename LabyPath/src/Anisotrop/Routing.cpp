/*
 * Routing.cpp
 *
 *  Created on: Mar 27, 2018
 *      Author: florian
 */

#include "Routing.h"

#include <cstdint>
#include <boost/heap/detail/stable_heap.hpp>
#include <boost/heap/pairing_heap.hpp>
#include <CGAL/Arr_extended_dcel.h>
#include <CGAL/Arr_geometry_traits/Curve_data_aux.h>
#include <CGAL/Arrangement_on_surface_2.h>
#include <CGAL/Point_set_2.h>
#include <easy/profiler.h>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include "../basic/LinearGradient.h"
#include "../basic/PairInteger.h"
#include "../basic/PolygonTools.h"
#include "../SegmentPS.h"

namespace laby {
namespace aniso {

Routing::Routing(Arrangement_2& arr, const proto::RoutingCost& config) :
        _config(config), //
        _arr(arr), //
        si(_convexList), //
        _random { 0, _config.max_random(), _config.seed() } {
    EASY_FUNCTION();
    _convexList.reserve(_arr.number_of_edges());

    edgesQList.reserve(_arr.number_of_edges());
    for (Vertex& ve : RangeHelper::make(_arr.vertices_begin(), _arr.vertices_end())) {
        ve.data().setId(edgesQList.size());
        edgesQList.emplace_back(ve);
    }

}

void Routing::connectTwoPinPath(const std::vector<aniso::Net>& nets, const SpatialIndex& si, std::vector<PolyConvex>& convexList) {
    std::unordered_map<const Vertex*, std::vector<std::size_t> > mapVertexPolyConvex;
    std::vector<const Vertex*> ordered_list; // predictable order

    for (const aniso::Net& net : nets) {
        if (net.isPlaced()) {

            {
                const aniso::Pin& pin1 = net.source();
                auto ite = mapVertexPolyConvex.try_emplace(&pin1.vertex());
                ite.first->second.emplace_back(pin1.polyConvexIndex());
                if (ite.second) { // actually inserted
                    ordered_list.emplace_back(&pin1.vertex());
                }
            }
            {
                const aniso::Pin& pin2 = net.target();
                auto ite = mapVertexPolyConvex.try_emplace(&pin2.vertex());

                ite.first->second.emplace_back(pin2.polyConvexIndex());
                if (ite.second) { // actually inserted
                    ordered_list.emplace_back(&pin2.vertex());
                }
            }
        }
    }
    for (const Vertex* vertexP : ordered_list) {

        const std::vector<std::size_t> &list = mapVertexPolyConvex.at(vertexP);
        uint32_t index = 0;

        if (list.size() == 1 and si.is_PointInside(convexList.at(list.at(0)), *vertexP, index)) {
            std::cout << "orphan PolyConvex connect not empty  list.at(0) index" << list.at(0) << " " << index << std::endl;
            PolyConvex::connect(list.at(0), index, convexList);
        }
        for (std::size_t i = 0; i < list.size(); ++i) {
            for (std::size_t j = i + 1; j < list.size(); ++j) {

                PolyConvex::connect(list.at(i), list.at(j), convexList, vertexP->point());

            }
        }
    }
}

void Routing::connectMaze(std::vector<PolyConvex>& polyConvexList) {

    // init structures

    std::cout << " connectMaze " << std::endl;

    std::vector<std::size_t> high_degree_polyConvex;
    std::unordered_set<basic::PairInteger> adjacence_list;

    CGAL::Union_find<std::size_t> uf;

    //make a Union find Structure on Polyconvexes
    for (const PolyConvex& pc : polyConvexList) {
        pc.handle = uf.push_back(pc._id);
    }

    // loop on polyconvex to unify adjacent Polyconvex
    for (const PolyConvex& pc : polyConvexList) {
        for (std::size_t id_adj : pc._adjacents) {
            uf.unify_sets(pc.handle, polyConvexList.at(id_adj).handle);
        }
    }

    //make a Point Set
    typedef CGAL::Point_set_2<Kernel> PS;
    PS pointSet;
    // with a map Vertex* -> list of PolyConvex
    std::unordered_map<const PS::Vertex*, std::vector<std::size_t>> map;
    std::vector<const PS::Vertex*> ordered_vertex;
    std::size_t statPoin = 0;
    std::size_t statNoPoint = 0;
    for (const PolyConvex& pc : polyConvexList) {

        if (pc.has_points()) {
            ++statPoin;
            const Point_2& p1 = pc.getSourcePoint();
            const Point_2& p2 = pc.getTargetPoint();

            const PS::Vertex* v1 = &*pointSet.insert(p1);
            const PS::Vertex* v2 = &*pointSet.insert(p2);
            {
                auto ite = map.try_emplace(v1);
                ite.first->second.emplace_back(pc._id);
                if (ite.second) {
                    ordered_vertex.emplace_back(v1);
                }
            }
            {
                auto ite = map.try_emplace(v2);
                ite.first->second.emplace_back(pc._id);
                if (ite.second) {
                    ordered_vertex.emplace_back(v2);
                }
            }
        } else {
            ++statNoPoint;
        }
    }
    std::cout << "statPoin " << statPoin << " statNoPoint " << statNoPoint << std::endl;

// for each element on map, loop on polyconvex list, and connect if distinct set

    std::cout << " connectMaze  1st connection begin" << std::endl;

    for (const PS::Vertex* v : ordered_vertex) {

        std::vector<std::size_t>& vec = map.at(v);
        if (!vec.empty()) {
            CGAL::Union_find<std::size_t>::handle& handle0 = polyConvexList.at(vec.at(0)).handle;
            for (std::size_t i = 1; i < vec.size(); ++i) {
                CGAL::Union_find<std::size_t>::handle& handle = polyConvexList.at(vec.at(i)).handle;
                if (!uf.same_set(handle0, handle)) {
                    std::cout << "PolyConvex::connect(vec.at(0), vec.at(i) " << vec.at(0) << " " << vec.at(i) << std::endl;
                    PolyConvex::connect(vec.at(0), vec.at(i), polyConvexList);

                    uf.unify_sets(handle0, handle);
                }
            }
        }
    }
    std::cout << " connectMaze  1st connection end" << std::endl;
// walk on all pair of point

    std::vector<SegmentPS> vectSeg;

    for (auto& edge : RangeHelper::make(pointSet.finite_edges_begin(), pointSet.finite_edges_end())) {
        PS::Face_handle & fh = edge.first;
        int i = edge.second;
        SegmentPS seg(&*fh->vertex(PS::cw(i)), &*fh->vertex(PS::ccw(i)));

        if (!uf.same_set(polyConvexList.at(map.at(seg._v1).front()).handle, polyConvexList.at(map.at(seg._v2).front()).handle)) {

            vectSeg.emplace_back(seg);
        }

    }
    std::sort(vectSeg.begin(), vectSeg.end());

    double thickness = 2.;

    for (const SegmentPS& seg : vectSeg) {
        std::size_t pcId1 = map.at(seg._v1).front();
        std::size_t pcId2 = map.at(seg._v2).front();
        CGAL::Union_find<std::size_t>::handle& handle1 = polyConvexList.at(pcId1).handle;
        CGAL::Union_find<std::size_t>::handle& handle2 = polyConvexList.at(pcId2).handle;
        if (!uf.same_set(handle1, handle2)) {

            // create a PolyConvex

            basic::LinearGradient lgrad(seg._v1->point(), thickness, seg._v2->point(), thickness);

            polyConvexList.emplace_back(seg._v1->point(), seg._v2->point(), polyConvexList.size(), lgrad);
            //warning : references are undefined now ( we have increase the size of the vector)

            PolyConvex& newPoly = polyConvexList.back();

            PolyConvex::connect(newPoly._id, pcId1, polyConvexList, seg._v1->point());
            PolyConvex::connect(newPoly._id, pcId2, polyConvexList, seg._v2->point());
            // connect with others

            uf.unify_sets(polyConvexList.at(pcId1).handle, polyConvexList.at(pcId2).handle);

            std::cout << " PolyConvex::connect( " << newPoly._id << " " << pcId1 << " " << pcId2 << std::endl;
        }
    }
    std::cout << " connectMaze  2nd  connection completed" << std::endl;
//end : reset handle just in case

    for (const PolyConvex& pc : polyConvexList) {
        pc.resetMutable();
    }

}

//experimental : need to work on placement of cut
void Routing::createMaze() {

// init structures

    std::vector<std::size_t> high_degree_polyConvex;
    std::unordered_set<basic::PairInteger> adjacence_list;

    CGAL::Union_find<std::size_t> uf;
    for (const PolyConvex& pc : _convexList) {
        if (pc._adjacents.size() > 2ul) {
            high_degree_polyConvex.emplace_back(pc._id);
        }
        for (std::size_t adjacence : pc._adjacents) {
            adjacence_list.emplace(pc._id, adjacence);
        }

        pc.handle = uf.push_back(pc._id);
    }

// create roots

// this loop could be optimize inside the init loop (to avoid erasing adjacence_list elements)
    for (std::size_t id : high_degree_polyConvex) {
        PolyConvex& pc = _convexList.at(id);
        for (std::size_t adjacence : pc._adjacents) {
            auto ite = adjacence_list.find(basic::PairInteger { pc._id, adjacence });
            if (ite != adjacence_list.end()) {
                uf.unify_sets(pc.handle, _convexList.at(adjacence).handle);
                adjacence_list.erase(ite);
            }
        }
    }

//should randomize access
    for (const basic::PairInteger& pair : adjacence_list) {
        PolyConvex& pc1 = _convexList.at(pair.first());
        PolyConvex& pc2 = _convexList.at(pair.second());
        CGAL::Union_find<std::size_t>::handle& handle1 = pc1.handle;
        CGAL::Union_find<std::size_t>::handle& handle2 = pc2.handle;
        if (uf.same_set(handle1, handle2)) {
            std::cout << "could cut between " << pair.first() << " and " << pair.second() << std::endl;
            pc1.remove_adjacence(pc2._id);
            pc2.remove_adjacence(pc1._id);

            pc2._geometry.clear();

        } else {
            uf.unify_sets(handle1, handle2);
        }
    }

//end : reset handle just in case

    for (const PolyConvex& pc : _convexList) {
        pc.resetMutable();
    }

}

void Routing::commitNewPath(const int32_t& targetId, Net& net) {
    Pin & pin1 = net.source();
    Pin & pin2 = net.target();
    std::size_t begin = _convexList.size();

    for (int32_t id = targetId; id != -1; id = edgesQList.at(id).parent) {

        PolyConvex& pc = edgesQList.at(id)._pc;
        if (!pc.empty()) {
            pc._id = _convexList.size();
            _convexList.emplace_back(pc);
            net.path().emplace_back(pc._id);
        }
    }

    PolyConvex::connect(begin, _convexList);
    pin1.setPolyConvexIndex(_convexList.size() - 1);

    pin2.setPolyConvexIndex(begin);
    for (std::size_t j = begin; j < _convexList.size(); ++j) {
        _convexList.at(j)._supportHe->curve().data().addPath(net.id());
        si.insert(_convexList.at(j));
    }
    net.markPlaced();
}

bool Routing::findRoute(Net & net) {
    EASY_FUNCTION();

    for (QueueElement& qe : edgesQList) {
        qe.clear();
    }

    PriorityQueue queue;

    const Pin& pin1 = net.source();
    const Pin& pin2 = net.target();
    const int32_t& sourceId = pin1.vertex().data().id();

    QueueElement& qEle = edgesQList.at(sourceId);
    const int32_t& targetId = pin2.vertex().data().id();
    basic::LinearGradient lgrad = net.gradient();

    qEle.pushIn(queue);
    qEle.cost.distance = 0;
    qEle.cost.via_num = 0;

    for (const Halfedge& he : RangeHelper::make(pin1.vertex().incident_halfedges())) {
        if (he.curve().data().congestion() != 0) {
            int32_t net_id = he.curve().data().get_net();
            qEle.cost.memory_source.emplace(net_id);
        }
    }

    std::unordered_set<int32_t> nets_target;
    for (const Halfedge& he : RangeHelper::make(pin2.vertex().incident_halfedges())) {
        if (he.curve().data().congestion() != 0) {
            int32_t net_id = he.curve().data().get_net();
            nets_target.emplace(net_id);
        }
    }

    PolyConvex pc;
    int32_t priority_number = 0;
    bool solved = false;
    while (!queue.empty()) {
        QueueElement & qEle = *queue.top();

        Vertex& vertex = qEle._vertex;
        if (vertex.data().id() == targetId) {
            std::cout << "END !!!!" << qEle.cost << "\n";
            solved = true;
            break;
        }
        queue.pop();
        qEle.resetHandle();

        int32_t degree = vertex.degree();

        for (Halfedge& he : RangeHelper::make(vertex.incident_halfedges())) {
            //avoid going back
            if (qEle._pc.empty() || &he.curve() != &qEle._pc._supportHe->curve()) {

                QueueCost cost = qEle.cost;

                cost.memory_source = qEle.cost.future_memory_source;

                cost.future_memory_source.clear();
                cost.memory_target = qEle.cost.future_memory_target;

                cost.future_memory_target.clear();

                if (degree > 2) {

                    cost.distance += _config.distance_unit_cost();
                }
                int32_t newDir = he.curve().data().direction();
                int32_t congestion = he.curve().data().congestion();
                cost.congestion += congestion;

                pc = PolyConvex(he, 0, lgrad);

                if (congestion == 0) {
                    si.update_cost_if_intersect(pc, net, nets_target, cost);
                }
                if (cost.congestion == 0) {
                    for (const int32_t i : cost.memory_target) {
                        if (cost.future_memory_target.count(i) == 0) {
                            ++cost.congestion;
                        }
                    }
                }
                if (qEle.direction != -1 && newDir != qEle.direction) {

                    cost.distance += _config.via_unit_cost();
                }

                int32_t newId = he.source()->data().id(); // vertex id

                QueueElement& newqEle = edgesQList.at(newId);
                if (cost.congestion == 0 and (newqEle.cost.distance == -1 or newqEle.cost > cost)) {
                    cost.randomization = _random.get(); //priority_number;
                    --priority_number;
                    newqEle.cost = cost;
                    newqEle.direction = newDir;
                    newqEle.parent = qEle._vertex.data().id();
                    newqEle._pc = pc;
                    if (newqEle.isInQueue()) {
                        queue.update(newqEle.handle);
                    } else {
                        newqEle.pushIn(queue);
                    }
                }
            }
        }
    }

    if (solved) {
        commitNewPath(targetId, net);
    }

    return solved;
}

} /* namespace aniso */
} /* namespace laby */
